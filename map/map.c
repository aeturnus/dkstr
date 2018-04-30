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

static void assign_weights(map_t* map, uint32_t* weight_buffer) {
    for (int r = 0; r < map->h; r++) {
        for (int c = 0; c < map->w; c++) {

            // TODO: slight optimizations
            //       - don't have to AND with a mask if already zero (just an OR will work)
            //       - don't need these if-statements --> can just loop over specific pieces of the map in groups

            int mb_index = r*map->w + c;

            // assign the top left edge
            if (c - 1 >= 0   &&   r - 1 >= 0) {
                int wb_index = mb_index - map->w - 1;
                map->buffer[mb_index] = (map->buffer[mb_index] & TL_MASK) | (weight_buffer[wb_index] << TL_SHIFT);
            }

            // assign the top edge
            if (r - 1 >= 0) {
                int wb_index = mb_index - map->w;
                map->buffer[mb_index] = (map->buffer[mb_index] & T_MASK) | (weight_buffer[wb_index] << T_SHIFT);
            }

            // assign the top right edge
            if (r - 1 >= 0   &&   c + 1 < map->w) {
                int wb_index = mb_index - map->w + 1;
                map->buffer[mb_index] = (map->buffer[mb_index] & TR_MASK) | (weight_buffer[wb_index] << TR_SHIFT);
            }

            // assign the left edge
            if (c - 1 >= 0) {
                int wb_index = mb_index - 1;
                map->buffer[mb_index] = (map->buffer[mb_index] & L_MASK) | (weight_buffer[wb_index] << L_SHIFT);
            }

            // assign the right edge
            if (c + 1 < map->w) {
                int wb_index = mb_index + 1;
                map->buffer[mb_index] = (map->buffer[mb_index] & R_MASK) | (weight_buffer[wb_index] << R_SHIFT);
            }
            
            // assign the bottom left edge
            if (c - 1 >= 0   &&   r + 1 < map->w) {
                int wb_index = mb_index - 1 + map->w;
                map->buffer[mb_index] = (map->buffer[mb_index] & BL_MASK) | (weight_buffer[wb_index] << BL_SHIFT);
            }
            
            // assign the bottom edge
            if (r + 1 < map->w) {
                int wb_index = mb_index + map->w;
                map->buffer[mb_index] = (map->buffer[mb_index] & B_MASK) | (weight_buffer[wb_index] << B_SHIFT);
            }
            
            // assign the bottom right edge
            if (r + 1 < map->w   &&   c + 1 < map->w) {
                int wb_index = mb_index + map->w + 1;
                map->buffer[mb_index] = (map->buffer[mb_index] & BR_MASK) | (weight_buffer[wb_index] << BR_SHIFT);
            }
        }
    }
}

static void print_map(map_t* map, uint32_t* weight_buffer) {
    for (int i = 0; i < map->h*map->w; i++) {
        if (weight_buffer[i] == MAX_WEIGHT) {
            printf("%*c", 3, '*'); // obstacles
        } else {
            //printf("%*d", 3, weight_buffer[i]); // weights
            printf("%*c", 3, ' '); // blank meaning some weight that isn't MAX_WEIGHT
        }
        if (i % map->w == 0) {
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
    uint32_t weight_buffer[rows*cols];
    for (int i = 0; i < rows*cols; i++) {
        weight_buffer[i] = rand() % (MAX_WEIGHT+1);
    }

    // print the map of random weights
//     print_map(map, weight_buffer);

    // assign the weights as "edges" to each cell
    assign_weights(map, weight_buffer);
    
    return map;
}

void delete_map(map_t* map) {
    free(map->buffer);
    free(map);
}

// TODO: move main() into map-test.c
int main(void) {
    map_t* map = generate_map(32, 32);
    delete_map(map);
}

