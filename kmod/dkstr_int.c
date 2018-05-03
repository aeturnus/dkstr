#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/ioport.h>

#include <linux/platform_device.h>
#include <linux/of.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#define MODULE_NAME "dkstr_int"
#define MODULE_HEAD MODULE_NAME ": "
#define MODULE_VER "1.0"

#define INTERRUPT_NAME "dkstr-interrupt"
#define MAJOR_NUM 243

#define DEBUG

static int int_cnt;     // counter for interrupts
static int linux_irqn;  // linux int number
static struct fasync_struct *fasync_dkstr_queue;

// /dev node
static int dkstr_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int dkstr_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int dkstr_fasync(int fd, struct file *filep, int on)
{
    return fasync_helper(fd, filep, on, &fasync_dkstr_queue);
}

static irqreturn_t dkstr_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    ++int_cnt;

    // signal user application with a SIGIO
    kill_fasync(&fasync_dkstr_queue, SIGIO, POLL_IN);
    return 0;
}

// /proc node
static int dkstr_proc_read(struct file *fp, char __user *buffer,
                          size_t length, loff_t *offset)
{
    int len = 0;
    int str_len;
    int ret;
    char * msg;

    static int state = 0;
    // return 0 on next read to make cat happy: NON RE-ENTRANT
    if (state == 1) {
        state = 0;
        return 0;
    }
    #define BUFFER_LEN 128
    msg = (char *) kmalloc(BUFFER_LEN, GFP_KERNEL);
    str_len = snprintf(msg, BUFFER_LEN, MODULE_HEAD "interrupt count: %d\n", int_cnt);
    ret = copy_to_user(buffer, msg, length);

    if (ret == 0)
        len = str_len;
    else
        len = str_len - length;

    kfree(msg);
    state = 1;
    return len;
    #undef BUFFER_LEN
}

// platform driver
static int dkstr_driver_probe(struct platform_device *pdev)
{
    struct resource *res;
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

    if (!res) {
        printk(KERN_ERR MODULE_HEAD "unable to find IRQ\n");
        return 0;
    }

    linux_irqn = res->start;
    printk(KERN_INFO MODULE_HEAD "probed IRQ: %d\n", linux_irqn);
    return 0;
}

static int dkstr_driver_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct file_operations dkstr_fops = {
    .owner  = THIS_MODULE,
    .llseek = NULL,
    .read   = NULL,
    .write  = NULL,
    .poll   = NULL,
    .unlocked_ioctl = NULL,
    .mmap   = NULL,
    .open   = dkstr_open,
    .flush  = NULL,
    .release= dkstr_release,
    .fsync  = NULL,
    .fasync = dkstr_fasync,
    .lock   = NULL,
    .read   = NULL,
    .write  = NULL,
};

static const struct file_operations proc_fops = {
    .owner  = THIS_MODULE,
    .read   = dkstr_proc_read,
};

static const struct of_device_id dkstr_of_match[] = {
    {.compatible = "xlnx,dkstra-last-1.0"},
    {}
};

MODULE_DEVICE_TABLE(of, dkstr_of_match);

static struct platform_driver dkstr_driver = {
    .driver = {
        .name = MODULE_NAME,
        .of_match_table = dkstr_of_match
    },
    .probe  = dkstr_driver_probe,
    .remove = dkstr_driver_remove
};

static struct proc_dir_entry *dkstr_int_file;
/*
 * Initializes the module
 * Creates the /proc node and registers interrupt INTERRUPT_NUM
 */
static int __init init_dkstr_int(void)
{
    int ret;

    int_cnt = 0;

    printk(KERN_INFO MODULE_HEAD "loading...\n");

    // register the dkstr platform device
    platform_driver_unregister(&dkstr_driver);
    ret = platform_driver_register(&dkstr_driver);
    if (ret) {
        printk(KERN_INFO MODULE_HEAD
               "registering driver returned with error %d\n", ret);

        goto fail_platform;
    }

    // register character device
    // install script should create the /dev node
    ret = register_chrdev(MAJOR_NUM, MODULE_NAME, &dkstr_fops);
    if (ret) {
        printk(KERN_ERR MODULE_HEAD "unable to get major num %d. Exiting...\n",
               MAJOR_NUM);
        goto fail_chrdev;
    }

    // setup /proc entry
    dkstr_int_file = proc_create(INTERRUPT_NAME, 0444, NULL, &proc_fops);
    if (dkstr_int_file == NULL) {
        printk(KERN_ERR MODULE_HEAD "unable to create /proc entry. Exiting...\n");
        goto fail_proc;
    }

    // setup interrupt
    ret = request_irq(linux_irqn, (irq_handler_t) dkstr_int_handler,
                      IRQF_TRIGGER_RISING, INTERRUPT_NAME, NULL);

    if (ret) {
        printk(KERN_ERR MODULE_HEAD "unable to get IRQ #%d\n", linux_irqn);
        goto fail_irq;
    }

    printk(KERN_INFO MODULE_HEAD "succesfully loaded driver\n");
    return 0;

fail_irq:
    remove_proc_entry(INTERRUPT_NAME, NULL);
fail_proc:
    unregister_chrdev(MAJOR_NUM, MODULE_NAME);
fail_chrdev:
    platform_driver_unregister(&dkstr_driver);
fail_platform:
    return -EBUSY;
}

/*
 * Cleans up the module for exiting
 */
static void __exit exit_dkstr_int(void)
{
    printk(KERN_INFO MODULE_HEAD "exiting...\n");

    free_irq(linux_irqn, NULL);
    remove_proc_entry(INTERRUPT_NAME, NULL);
    unregister_chrdev(MAJOR_NUM, MODULE_NAME);
    platform_driver_unregister(&dkstr_driver);
}

module_init(init_dkstr_int);
module_exit(exit_dkstr_int);

MODULE_AUTHOR("Brandon Nguyen");
MODULE_DESCRIPTION("dkstr_int proc module");
MODULE_LICENSE("GPL");
