#ifndef __WORLD_H__
#define __WORLD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct coord_
{
    int x;
    int y;
} coord;

typedef struct entity_
{
    coord pos;
} entity;

typedef struct movement_
{
    int count;
    int x_dir;
    int y_dir;
} movement;


#ifdef __cplusplus
}
#endif

#endif//__WORLD_H__
