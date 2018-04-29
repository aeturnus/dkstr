#ifndef __MAP_H__
#define __MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// maps have a width and height, composed of width*height tiles
//
// the buffer contains the weights to travel to a particular tile
// unlike typical representations where the weights are in each edge,
// we compress this information by having the weight of an edge
// be the cost to travel to that tile: this allows for one single
// value to encode 8 edges (the ones pointing to the tile)
//
// the buffer is in row major form
// i.e. index = column + row * width
//
// For the sake of the profiler, assume 32x32 maps
// Weights are 4 bits, so they max out at 15
// A weight of 15 represents that this tile is an obstruction and cannot be traveled to.
// You can travel horizontally and vertically: this incurs a base cost of 1.0
// You can travel diagonally. This incurs a base cost of 1.5
// (we handle the .5 by using the Q11.1 fixed point format in the accelerator:
//  don't worry, we handle the shift in order to add weights)
// The weights are added on top of this.
// For your sanity, let each uint8_t be a weight (no packing)
// I (Brandon) can handle writing the packer that sends the data off
//
// The map generator needs to:
// - generate a 32x32 map in row-major format
// - ensure that any valid (not on an obstruction) start and end point will have a path.
//      - enforce this by requiring that all maps have a single partition of unobstructed area
//        if you have unobstructed areas surrounded by a ring of obstruction, just fill those in with obstruction
// The test case generator needs to:
// - pick a random start and end position. If they're invalid (on obstruction), then try again.
//   By enforcing map layouts in the generator, this logic becomes easier to do

typedef struct map_ {
    int w;
    int h;
    uint32_t* buffer;
} map_t;

#ifdef __cplusplus
}
#endif

#endif//__MAP_H__
