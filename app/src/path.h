#ifndef __PATH_H__
#define __PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <btn/vector.h>

#include "world.h"

typedef struct path_
{
    // stack of movements: pop them
    vector(movement) moves;
} path;

void path_ctor(path * path);
void path_dtor(path * path);
void path_find(const map * map, const coord * start, const coord * end,
               path * path);
void ppath_find(const map * map, const coord * start, const coord * end,
               path * path);

void path_load(const map * map, const coord * start, const coord * end,
               const uint32_t * buffer, path * path);

#ifdef __cplusplus
}
#endif

#endif//__PATH_H__
