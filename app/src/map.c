#include <stdio.h>
#include <stdlib.h>
#include "map.h"

int map_load(map * map, const char * file_name)
{
    FILE * file = fopen(file_name, "r");
    if (file == NULL)
        return 1;
    char buf[64];

    fgets(buf, sizeof(buf), file);

    sscanf(buf, "%d %d", &map->h, &map->w);
    map->buffer = (char *) malloc(sizeof(char) * map->h * map->w);

    char * line = (char *) malloc(sizeof(char) * map->w + 1);
    for (int r = 0; r < map->h; r++) {
        fgets(line, sizeof(char) * map->w + 1, file);
        for (int c = 0; c < map->w; c++) {
            //map->buffer[c + map->w * r] = line[c];
            map_put(map, c, r, line[c]);
        }
        // second one to take care of the newline
        fgets(line, sizeof(char) * map->w + 1, file);
    }

    free(line);
    fclose(file);

    return 0;
}

void map_copy(map * dst, const map * src)
{
    dst->w = src->w;
    dst->h = src->h;
    dst->buffer = (char *) malloc(sizeof(char) * src->h * src->w);
    for (int r = 0; r < src->h; ++r) {
        for (int c = 0; c < src->w; ++c) {
            map_put(dst, c, r, map_get(src, c, r));
        }
    }
}

void map_dtor(map * map)
{
    free(map->buffer);
}

static void map_gen_border(map * map) 
{
    // top border
    for (int i = 1; i < map->w - 1; i++) {
        map->buffer[i] = '-';
    }

    // bottom border
    for (int i = 1; i < map->w - 1; i++) {
        int offset = (map->h - 1) * map->w;
        map->buffer[i + offset] = '-';
    }

    // left border
    for (int i = 0; i < map->h * map->w; i += map->w) {
        map->buffer[i] = '|';
    }

    // right border
    for (int i = map->w - 1; i < map->h * map->w; i += map->w) {
        map->buffer[i] = '|';
    }
}

void map_rand(map * map, int seed, int width, int height) 
{
    // initialize the map struct
    map->w = width;
    map->h = height;

    // seed the random number generator
    srand(seed);

    // fill in the border
    map_gen_border(map);

    // generate weighted random symbols for each cell in the map
    // 80% for ' ', 10% for '#', 10% for '@'
    int total_tickets = 100;
#define NUM_SYMBOLS   3 // TODO: make this parametrized?
    int symbols[NUM_SYMBOLS] = { ' ', '#', '@' }; // TODO: make this parametrized?
    double weights[NUM_SYMBOLS] = { 0.24, 0.38, 0.38 }; // TODO: make this parametrized?
    double tickets[NUM_SYMBOLS];
    for (int i = 0; i < NUM_SYMBOLS; i++) {
        tickets[i] = weights[i] * total_tickets;
    }
    for (int row_i = 1; row_i < map->h - 1; row_i++) {
        for (int col_i = 1; col_i < map->w - 1; col_i++) {
            double rand_tickets = rand() % total_tickets;
            int ticket_i;
            for (ticket_i = 0; ticket_i < NUM_SYMBOLS; ticket_i++) {
                if (rand_tickets < tickets[ticket_i]) {
                    break;
                }
                rand_tickets -= tickets[ticket_i];
            }
            map->buffer[row_i * map->w + col_i] = symbols[ticket_i];
        }
    }
}

// test main checking the random map generation function map_rand()
/*int test_main(void) {

    int seed = 1;
    int width = 28;
    int height = 28;
    
    map map;
    map.buffer = (uint8_t*) malloc(width * height * sizeof(uint8_t));
    map_rand(&map, seed, width, height);
    
    for (int i = 0; i < map.w * map.h; i++) {
        printf("%c", map.buffer[i]);
        if (i % map.w == map.w-1) {
            printf("\n");
        }
    }
}*/
