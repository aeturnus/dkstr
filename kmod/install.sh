#!/bin/bash
insmod dksr_int.ko
mknod /dev/dkstr_int c 243 0
