#ifndef __MAP_H__
#define __MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct map_
{
    int w;
    int h;
    char * buffer;
} map;

//__attribute__((always_inline))
static inline
char map_get(const map * map, int x, int y)
{
    return map->buffer[x + map->w * y];
}

static inline
void map_put(map * map, int x, int y, char c)
{
    map->buffer[x + map->w * y] = c;
}

// returns 0 on success
int map_load(map * map, const char * file_name);
void map_copy(map * dst, const map * src);
void map_dtor(map * map);

#ifdef __cplusplus
}
#endif

#endif//__MAP_H__
