/* map.c */

#include "map-spec.h"
#include <stdlib.h>
#include <stdio.h> // TODO: remove

#define MAP_SEED     42

#define MAX_WEIGHT   15

#define TL_SHIFT     0
#define T_SHIFT      4
#define TR_SHIFT     8
#define L_SHIFT      12
#define R_SHIFT      16
#define BL_SHIFT     20
#define B_SHIFT      24
#define BR_SHIFT     28

#define TL_MASK      0xFFFFFFF0
#define T_MASK       0xFFFFFF0F
#define TR_MASK      0xFFFFF0FF
#define L_MASK       0xFFFF0FFF
#define R_MASK       0xFFF0FFFF
#define BL_MASK      0xFF0FFFFF
#define B_MASK       0xF0FFFFFF
#define BR_MASK      0x0FFFFFFF

static void print_map(map_t* map) {
    for (int i = 0; i < map->h*map->w; i++) {
        if (map->buffer[i] == MAX_WEIGHT) {
            printf("%*c", 3, '*'); // obstacles
        } else {
            printf("%*d", 3, map->buffer[i]); // weights
            //printf("%*c", 3, ' '); // blank meaning some weight that isn't MAX_WEIGHT
        }
        if (i % map->w == map->w-1) {
            printf("\n");
        }
    }
}

map_t* generate_map(int rows, int cols) {

    // initialize the map struct
    map_t* map = (map_t*) malloc(sizeof(map));
    map->w = cols;
    map->h = rows;
    map->buffer = (uint32_t*) malloc(rows * cols * sizeof(uint32_t));

    // seed the random number generator
    srand(MAP_SEED);

    // generate random weights for each cell in the map
    for (int i = 0; i < rows*cols; i++) {
        map->buffer[i] = rand() % (MAX_WEIGHT+1);
    }
    
    return map;
}

void delete_map(map_t* map) {
    free(map->buffer);
    free(map);
}

// TODO: move main() into map-test.c
int main(void) {
    map_t* map = generate_map(32, 32);
    print_map(map);
    delete_map(map);
}

