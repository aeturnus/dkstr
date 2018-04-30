/* map.c */

#include "map-spec.h"
#include <stdlib.h>
#include <stdio.h>

#define MAP_SEED     4

#define MAX_WEIGHT   15

#define NUM_WEIGHTS_PROBS   4

map_t* generate_map(int rows, int cols) {

    // initialize the map struct
    map_t* map = (map_t*) malloc(sizeof(map));
    map->w = cols;
    map->h = rows;
    map->buffer = (uint8_t*) malloc(rows * cols * sizeof(uint8_t));

    // seed the random number generator
    srand(MAP_SEED);

    // generate weighted random weights for each cell in the map
    // 40% for 0, 20% for 1, 30% for MAX_WEIGHT, 10% total for 2 to (MAX_WEIGHT-1)
    // sum = 96 + 48 + 72 + 24 = 240
    int weight_sum = 240;
    int vals[MAX_WEIGHT+1] = { 0, 1, MAX_WEIGHT };
    double weights_probs[NUM_WEIGHTS_PROBS] = { 0.4, 0.2, 0.3, 0.1 };
    int weighted_vals[NUM_WEIGHTS_PROBS] = { (int)(weights_probs[0] * weight_sum), 
                                             (int)(weights_probs[1] * weight_sum), 
                                             (int)(weights_probs[2] * weight_sum), 
                                             (int)(weights_probs[3] * weight_sum) };
    for (int buffer_i = 0; buffer_i < rows*cols; buffer_i++) {
        int rand_weight_sum = rand() % weight_sum;
        int weight_i;
        for (weight_i = 0; weight_i < NUM_WEIGHTS_PROBS; weight_i++) {
            if (rand_weight_sum < weighted_vals[weight_i]) {
                break;
            }
            rand_weight_sum -= weighted_vals[weight_i];
        }
        if (weight_i < NUM_WEIGHTS_PROBS-1) {
            map->buffer[buffer_i] = vals[weight_i];
        } else {
            map->buffer[buffer_i] = (rand() % 13) + 2; // TODO: fix this b/c it's hardcoded but really 
                                                       //       this whole weighted random number generation logic is hardcoded
        }
    }

    return map;
}

static void print_map(map_t* map) {
    for (int i = 0; i < map->h*map->w; i++) {
        if (map->buffer[i] == MAX_WEIGHT) {
            printf("%*c", 1, '*'); // obstacles
        } else {
            //printf("%*d", 3, map->buffer[i]); // weights
            printf("%*c", 1, ' '); // blank meaning some weight that isn't MAX_WEIGHT
        }
        if (i % map->w == map->w-1) {
            printf("\n");
        }
    }
}

void delete_map(map_t* map) {
    free(map->buffer);
    free(map);
}

loc_t* get_empty_loc(map_t* map) {
    loc_t* loc = (loc_t*) malloc(sizeof(loc_t));
    while (1) {
        loc->r = rand() % map->h;
        loc->c = rand() % map->w;
        if (map->buffer[loc->r * map->w + loc->c] != MAX_WEIGHT) { // if not starting on an obstacle, then done generating locations
            break;
        }
    }
    return loc;
}

void delete_loc(loc_t* loc) {
    free(loc);
}

int main(void) {

    map_t* map = generate_map(32, 32);
    loc_t* start_loc = get_empty_loc(map);
    loc_t* end_loc = get_empty_loc(map);

    print_map(map);
    printf("start location (row, col): (%d, %d)\n", start_loc->r, start_loc->c);
    printf("end location (row, col): (%d, %d)\n", end_loc->r, end_loc->c);

    delete_map(map);
    delete_loc(start_loc);
    delete_loc(end_loc);
}

