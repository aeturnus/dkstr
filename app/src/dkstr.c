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


    map map;
    map_load(&map, map_path);

    uint32_t * bram = mem_addr(&mem_bram, (void*)(uintptr_t) 0x40000000);
    convert_map(&map, bram);

    map_dtor(&map);
    mem_dtor(&mem_bram);
    return 0;
}

int hw_pathfind(const map * map, const coord * start, const coord * end, path * path)
{
    // since 8 node weights fit into a single word, figure out how
    // many words we need
    int buff_size = 0;
    buff_size = (map->w * map->h) / 8;
    if ((map->w * map->h) % 8 != 0)
        buff_size += 1; // compensate if not aligned

    uint32_t * buffer = (uint32_t *) malloc(sizeof(uint32_t) * buff_size);

    convert_map(map, buffer);

    free(buffer);
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
    if (hw) {
        /// TODO
        hw_pathfind(&map, start, end, &path);
    }
    else {
        path_find(&map, start, end, &path);
    }
    ncurses_play(&map, &path, start);

    path_dtor(&path);
    return 0;
}

int main(int argc, char * argv[])
{
    printf("Hello world!\n");

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
    // dkstr play <map_path> <start_x> <start_y> <end_x> <end_y> [sw,hw; default sw]
    else if (!strcmp("play", argv[1])) {
        if (argc <= 6) {
            fprintf(stderr, "ERROR: dkstr play <start_x> <start_y> <end_x> <end_y> [sw,hw; default sw]\n");
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
}
