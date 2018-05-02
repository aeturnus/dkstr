#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>

#include <ncurses.h>
#include <mem/mem.h>

#include "map.h"
#include "world.h"
#include "path.h"

#include "prof.h"

extern const int32_t cost_table[128];
void draw_map(const map * map)
{
    for (int r = 0; r < map->h; r++) {
        for (int c = 0; c < map->w; c++) {
            mvaddch(r, c, map_get(map, c, r));
        }
    }
}

void draw_entity(const entity * e)
{
    mvaddch(e->pos.y, e->pos.x, '1');
    move(0,0);
}

void ncurses_play(const map * map, const path * path,
                  const coord * start)
{
    entity e = {.pos = *start};
    initscr();
    draw_map(map);
    draw_entity(&e);
    refresh();

    int cost = 0;
    for (int i = vector_size(&(path->moves)) - 1; i >= 0; --i) {
        movement move;
        vector_get(&(path->moves), i, &move);
        for (int i = 0; i < move.count; ++i) {
            if (move.x_dir != 0 && move.y_dir != 0)
                cost += (1 << 1) | 1;
            else
                cost += (1 << 1) | 0;

            e.pos.x += move.x_dir;
            e.pos.y += move.y_dir;

            cost += (cost_table[map_get(map, e.pos.x, e.pos.y)] << 1);

            draw_map(map);
            draw_entity(&e);
            refresh();
            //usleep(250000);
            usleep(100000);
        }
    }
    getch();
    endwin();

    printf("Total cost: %d.%d\n", cost >> 1, (cost & 1) ? 5 : 0);
}

void convert_map(const map * map, uint32_t * buffer)
{
    uint32_t value = 0;
    int count = 0;
    for (int r = 0; r < map->w; ++r) {
        for (int c = 0; c < map->h; ++c) {
            uint8_t cost;
            if (cost_table[map_get(map,c,r)] == 0xDEADBEEF)
                cost = 0xF;
            else
                cost = cost_table[map_get(map,c,r)] & 0xF;

            value |= cost << (count * 4);

            if (count == 7) {
                count = 0;
                *buffer = value;
                ++buffer;
                value = 0;
            } else {
                ++count;
            }
        }
    }
}

int dump_map(const char * map_path)
{
    mem_context mem_bram;
    mem_ctor(&mem_bram, MEM_MMAP, 1, (void*)(uintptr_t) 0x40000000, (void*)(uintptr_t) 0x40000fff);
    uint32_t * bram = mem_addr(&mem_bram, (void*)(uintptr_t) 0x40000000);

    map map;
    map_load(&map, map_path);

    convert_map(&map, bram);

    map_dtor(&map);
    mem_dtor(&mem_bram);
    return 0;
}

int dump_path(const char * path_path)
{
    mem_context mem_bram;
    mem_ctor(&mem_bram, MEM_MMAP, 1, (void*)(uintptr_t) 0x40001000, (void*)(uintptr_t) 0x40001fff);
    uint32_t * bram = mem_addr(&mem_bram, (void*)(uintptr_t) 0x40001000);

    FILE * file = fopen(path_path, "r");
    uint32_t val = 0;
    while (fscanf(file, "%08x", &val)) {
        *bram = val;
        ++bram;
    }
    fclose(file);

    mem_dtor(&mem_bram);
}

// gets the 4-bit dir code from a buffer
//#define hw_get(buffer,x,y) ( ((x)*(y)))
static inline
uint8_t hw_get(const uint32_t * buffer, int w, int h, int x, int y)
{
    // convert into a linear index into w*h*8 4-bit buffer
    int lindex = x + y *h;
    int index = lindex / 8;
    return (buffer[index] >> ((lindex % 8) * 4)) & 0xF;
}

const int dirs[8][2] =
{
    { 0,-1},// north
    { 1,-1},// northeast
    { 1, 0},// east
    { 1, 1},// southeast
    { 0, 1},// south
    {-1, 1},// southwest
    {-1, 0},// west
    {-1,-1},// northwest
};

void hw_gen_path(int w, int h, const coord * start, const coord * end,
                const uint32_t * buffer, path * path)
{
    path_ctor(path);

    uint8_t dir = 0;
    uint8_t prev_dir = 0xFF;
    coord curr = *end;
    while (!(curr.x == start->x && curr.y == start->y)) {
        dir = hw_get(buffer, w, h, curr.x, curr.y);
        if (dir & 0x8)
            dir &= 0x7;
        else {
            //printf("No path from (%d,%d) to (%d,%d)\n", start->x, start->y, end->x, end->y);
            return;
        }

        /*
        printf("Going from (%d,%d) -> (%d,%d)\n",
               curr.x, curr.y,
               curr.x + dirs[dir][0], curr.y + dirs[dir][1]);
        */

        if (dir == prev_dir) {
            movement * move_p = (movement *) vector_backp(&path->moves);
            move_p->count += 1;
        }
        else {
            movement move;
            move.count = 1;
            // reverse since we're moving backwards
            move.x_dir = -dirs[dir][0];
            move.y_dir = -dirs[dir][1];
            vector_push_back(&path->moves, &move);
        }
        curr.x = curr.x + dirs[dir][0];
        curr.y = curr.y + dirs[dir][1];

        prev_dir = dir;
    }
}

#define CTRL_RUN (1 << 31)
#define CTRL_LD  (1 << 30)
#define CTRL_X_MASK (0x1F)
#define CTRL_Y_MASK (0x1F)
#define CTRL_Y_SHF (5)
#define CTRL_X_SHF (0)
int hw_pathfind(const map * map, const coord * start, const coord * end, path * path,
                uint32_t * bram_map, uint32_t * bram_dir, volatile uint32_t * dkstr,
                prof * prof)
{
    prof_start(prof);
    // since 8 node weights fit into a single word, figure out how
    // many words we need
    int buff_n = 0;
    buff_n = (map->w * map->h) / 8;
    if ((map->w * map->h) % 8 != 0)
        buff_n += 1; // compensate if not aligned

    uint32_t * map_buffer = (uint32_t *) malloc(sizeof(uint32_t) * buff_n);

    convert_map(map, map_buffer);
    prof_end(prof); prof->prproc += prof_dt(prof);

    // transfer node weights to bram
    prof_start(prof);
    memcpy(bram_map, map_buffer, sizeof(uint32_t) * buff_n);
    prof_end(prof); prof->tx += prof_dt(prof);

    // setup the ctrl reg value and program
    prof_start(prof);
    uint32_t ctrl = CTRL_RUN | CTRL_LD |
                    (start->y & CTRL_Y_MASK) << CTRL_Y_SHF |
                    (start->x & CTRL_X_MASK) << CTRL_X_SHF;
    *dkstr = ctrl;

    // poll
    int loops = 0;
    while ((*dkstr) & CTRL_RUN)
        ++loops;
    prof_end(prof); prof->exec += prof_dt(prof);
    printf("poll loops: %d\n", loops);

    ///*
    // transfer back
    prof_start(prof);
    uint32_t * dir_buffer = (uint32_t *) malloc(sizeof(uint32_t) * buff_n);
    memcpy(dir_buffer, bram_dir, sizeof(uint32_t) * buff_n);
    prof_end(prof); prof->rx += prof_dt(prof);
    prof_start(prof);
    hw_gen_path(map->w, map->h, start, end, dir_buffer, path);
    prof_end(prof); prof->poproc += prof_dt(prof);
    free(dir_buffer);
    //*/
    /*
    // work with BRAM directly
    prof_start(prof);
    hw_gen_path(map->w, map->h, start, end, bram_dir, path);
    // could also load path: slower
    // path_load(map, start, end, bram_dir, path);
    prof_end(prof); prof->poproc += prof_dt(prof);
    */

    uint32_t ld_cycles = dkstr[1];
    uint32_t run_cycles = dkstr[2];
    uint32_t st_cycles = dkstr[3];

    /*
    printf("Cycles spent loading the nodes: %d\n", ld_cycles);
    printf("Cycles spent executing the nodes: %d\n", run_cycles);
    printf("Cycles spent storing the nodes: %d\n", st_cycles);
    */
    /*
    printf("Time spent loading the nodes: %d ns\n", (uint64_t)ld_cycles);
    printf("Time  spent executing the nodes: %d ns\n", (uint64_t)run_cycles);
    printf("Time spent storing the nodes: %d ns\n", (uint64_t)st_cycles);
    */

    free(map_buffer);
    return 0;
}

int play_map(const char * map_path, int hw, const coord * start, const coord * end)
{
    map map;
    path path;

    if (map_load(&map, map_path)) {
        fprintf(stderr, "ERROR: unable to open map %s\n", map_path);
        return 1;
    }

    prof prof;
    prof_ctor(&prof);
    if (hw) {
        /// TODO
        mem_context mem_bram, mem_dkstr;
        mem_ctor(&mem_bram, MEM_MMAP, 1, (void*)(uintptr_t) 0x40000000, (void*)(uintptr_t) 0x40001fff);
        mem_ctor(&mem_dkstr, MEM_MMAP, 1, (void*)(uintptr_t) 0x40004000, (void*)(uintptr_t) 0x40004fff);
        uint32_t * bram_map = mem_addr(&mem_bram, (void*)(uintptr_t) 0x40000000);
        uint32_t * bram_dir = mem_addr(&mem_bram, (void*)(uintptr_t) 0x40001000);
        uint32_t * dkstr = mem_addr(&mem_dkstr, (void*)(uintptr_t) 0x40004000);
        hw_pathfind(&map, start, end, &path, bram_map, bram_dir, dkstr, &prof);
        mem_dtor(&mem_bram);
        mem_dtor(&mem_dkstr);
    }
    else {
        path_find(&map, start, end, &path, &prof);
    }
    ncurses_play(&map, &path, start);

    prof_print(&prof);
    path_dtor(&path);
    return 0;
}

int main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ERROR: please provide command\n");
        return 1;
    }

    if (!strcmp("dump", argv[1])) {
        if (argc <= 2) {
            fprintf(stderr, "ERROR: dkstr dump <map path>\n");
            return 1;
        }
        return dump_map(argv[2]);
    }
    else if (!strcmp("dump_path", argv[1])) {
        if (argc <= 2) {
            fprintf(stderr, "ERROR: dkstr dump <path hexdump path>\n");
            return 1;
        }
        return dump_path(argv[2]);
    }
    // dkstr play <map_path> <start_x> <start_y> <end_x> <end_y> [sw,hw; default sw]
    else if (!strcmp("play", argv[1])) {
        if (argc <= 6) {
            fprintf(stderr, "ERROR: dkstr play <map_path> <start_x> <start_y> <end_x> <end_y> [sw,hw; default sw]\n");
        }

        coord start, end;
        sscanf(argv[3], "%d", &start.x);
        sscanf(argv[4], "%d", &start.y);
        sscanf(argv[5], "%d", &end.x);
        sscanf(argv[6], "%d", &end.y);

        int hw = 0;
        if (argc > 7 && !strcmp("hw",argv[7]))
            hw = 1;

        return play_map(argv[2], hw, &start, &end);
    }
    else {
        fprintf(stderr, "ERROR: invalid command %s\n", argv[1]);
        return 1;
    }
}
