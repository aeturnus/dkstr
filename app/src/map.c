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
