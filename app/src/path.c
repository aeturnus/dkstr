#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map.h"
#include "path.h"
#include "world.h"

//#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#define dprintf(str, ...)

extern const int32_t cost_table[128];

void path_ctor(path * path)
{
    vector_ctor(&path->moves, sizeof(movement), NULL, NULL);
}

void path_dtor(path * path)
{
    vector_dtor(&path->moves);
}

// nodes contain a 28 bit cost and 4 bits for direction
#define DIR_R  1
#define DIR_L -1
#define DIR_U -1
#define DIR_D  1
#define DIR_H  0    // halt
#define DIR_N  -2   // NULL direction
typedef struct node_
{
    uint32_t cost : 26;
    int32_t dir_x: 2;
    int32_t dir_y: 2;
    int32_t visit: 1;
    int32_t queue: 1;
} node;

typedef struct graph_
{
    int w;
    int h;
    node * buffer;
} graph;

static inline
node * graph_ref(const graph * graph, int x, int y)
{
    return &(graph->buffer[x + graph->w * y]);
}

static inline
node graph_get(const graph * graph, int x, int y)
{
    return graph->buffer[x + graph->w * y];
}

static inline
void graph_put(graph * graph, int x, int y, node c)
{
    graph->buffer[x + graph->w * y] = c;
}

#define graph_node(graph,x,y) (graph)->buffer[x + (graph)->w * y]

//__attribute__((always_inline))
static inline
int32_t calc_cost(char c)
{
    return cost_table[c];
}
//#define calc_cost(c) cost_table[c]

// initialize a graph scratchpad
// unvisited nodes have a direction of {0,0}
// unusable nodes have a direction of {-2,-2}
static
void gen_graph(graph * graph, const map * map)
{
    graph->w = map->w;
    graph->h = map->h;
    graph->buffer = (int *) malloc(sizeof(node) * graph->w * graph->h);

    for (int x = 0; x < map->w; ++x) {
        for (int y = 0; y < map->h; ++y) {
            node n = {.cost = -1, .dir_x = DIR_H, .dir_y = DIR_H, .visit = 0, .queue = 0};
            graph_put(graph, x, y, n);
        }
    }
}

static const int dirs[8][3] =
{
    // x  ,  y,    cost (Q31.1)
    {DIR_L, DIR_U, 0x3},
    {DIR_H, DIR_U, 0x2},
    {DIR_R, DIR_U, 0x3},
    {DIR_R, DIR_H, 0x2},
    {DIR_R, DIR_D, 0x3},
    {DIR_H, DIR_D, 0x2},
    {DIR_L, DIR_D, 0x3},
    {DIR_L, DIR_H, 0x2}
};

// return true if all nodes visitied
bool check_nodes_visited(graph * g)
{
    int node_cnt = g->w * g->h;
    for (int i = 0; i < node_cnt; ++i) {
        if (!g->buffer[i].visit)
            return false;
    }

    return true;
}

typedef struct queue_
{
    coord * buffer;
    int head;
    int enq_idx;
    int deq_idx;
    int cap;
    int size;
} queue;

static
void queue_ctor(queue * q, graph * g)
{
    q->cap = g->w * g->h;
    q->buffer = (coord *) malloc(sizeof(coord) * q->cap);
    q->enq_idx = 0;
    q->deq_idx = 0;
    q->size    = 0;
}

static
bool queue_enq(queue * q, const coord * c)
{
    if (q->size == q->cap)
        return false;

    //fprintf(stderr, "enqueueing (%d, %d)\n", c->x, c->y);
    q->buffer[q->enq_idx] = *c;
    q->enq_idx = (q->enq_idx + 1) % q->cap;
    q->size += 1;
    //fprintf(stderr, "queue size: %d\n", q->size);
    return true;
}

static
bool queue_deq(queue * q, coord * c)
{
    if (q->size == 0)
        return false;

    //fprintf(stderr, "dequeueing (%d, %d)\n", c->x, c->y);
    *c = q->buffer[q->deq_idx];
    q->deq_idx = (q->deq_idx + 1) % q->cap;
    q->size -= 1;
    //fprintf(stderr, "queue size: %d\n", q->size);
    return true;
}

static void gen_path(graph * graph, const coord * start, const coord * end,
                     path * path)
{
    coord curr = *end;
    int x_dir = -1;
    int y_dir = -1;
    int last_x_dir = 0;
    int last_y_dir = 0;
    int count = 0;

    dprintf("Going from (%d, %d) -> (%d, %d)\n",
            end->x, end->y, start->x, start->y);
    while (!(curr.x == start->x && curr.y == start->y)) {
        // follow the directions
        x_dir = graph_node(graph, curr.x, curr.y).dir_x;
        y_dir = graph_node(graph, curr.x, curr.y).dir_y;
        if (x_dir == 0 && y_dir == 0) {
            printf("failed");
            return;
        }

        coord next = {.x = curr.x + x_dir, .y = curr.y + y_dir};

        dprintf("Move: dx=%d, dy=%d (%d, %d) -> (%d, %d)\n", x_dir, y_dir,
                curr.x, curr.y, next.x, next.y);

        // coalesce into one movement
        if (x_dir == last_x_dir && y_dir == last_y_dir) {
            movement * move_p = (movement *) vector_backp(&path->moves);
            move_p->count += 1;
        } else {
            movement move;
            move.count = 1;
            // reverse since we're working backwards
            move.x_dir = -x_dir;
            move.y_dir = -y_dir;
            vector_push_back(&path->moves, &move);
        }

        last_x_dir = x_dir;
        last_y_dir = y_dir;

        curr = next;
        count++;
    }

    /*
    for (int i = vector_size(&path->moves) - 1; i >= 0; --i) {
        movement move;
        vector_get(&path->moves, i, &move);
        dprintf("Move: dx=%d, dy=%d  x%d\n", move.x_dir, move.y_dir, move.count);
    }
    */
}

// algorithm
//

// utilize the map to generate paths
void path_find(const map * map, const coord * start, const coord * end,
               path * path)
{
    queue queue;
    graph graph;

    path_ctor(path);
    gen_graph(&graph, map);
    queue_ctor(&queue, &graph);

    coord curr = *start;
    graph_node(&graph, curr.x, curr.y).cost = 0;

    queue_enq(&queue, &curr);

    while (queue_deq(&queue, &curr)) {
        graph_node(&graph, curr.x, curr.y).queue = 0;
        dprintf("curr: (%d, %d)\n", curr.x, curr.y);
        for (int i = 0; i < 8; i += 1) {
            coord next = {.x = curr.x + dirs[i][0], .y = curr.y + dirs[i][1]};
            uint32_t cost = dirs[i][2];

            // early return if out of bounds or boundary
            if (next.x < 0 || next.x >= graph.w ||
                next.y < 0 || next.y >= graph.h)
                continue;

            char tile = map_get(map, next.x, next.y);
            cost += calc_cost(tile) << 1;   // compensate for tile costs not being fixed point
            dprintf("    next tile: %c  (0x%08x)\n", tile, calc_cost(tile));
            if (calc_cost(tile) == 0xDEADBEEF)
                continue;
            cost += graph_node(&graph, curr.x, curr.y).cost;   // get the start pos cost

            dprintf("    next: (%d, %d) = %d.%d\n", next.x, next.y, cost >> 1, (cost & 1) ? 5 : 0);
            // redirect that node to current node if it costs less to move
            if (cost < graph_node(&graph, next.x, next.y).cost) {
                node * n = graph_ref(&graph, next.x, next.y);
                n->cost = cost;
                n->dir_x = curr.x - next.x;
                n->dir_y = curr.y - next.y;
                n->visit = 0;
                dprintf("        updated\n");
            }

            if (graph_node(&graph, next.x, next.y).visit == 0 &&
                graph_node(&graph, next.x, next.y).queue == 0) {
                graph_node(&graph, next.x, next.y).queue = 1;
                queue_enq(&queue, &next);
            }
        }
        graph_node(&graph, curr.x, curr.y).visit = 1;
    }

    // generate the path
    gen_path(&graph, start, end, path);
}

void ppath_find(const map * map, const coord * start, const coord * end,
                path * path)
{
    graph graph;

    path_ctor(path);
    gen_graph(&graph, map);

    graph_node(&graph, start->x, start->y).cost = 0;

    // anytime a node has to change, set run to 1
    int run;
    int count = 0;
    do {
        ++count;
        run = 0;
        coord curr;
        for (curr.y = 0; curr.y < graph.h; ++curr.y) {
            for (curr.x = 0; curr.x < graph.w; ++curr.x) {
                if (calc_cost(map_get(map, curr.x, curr.y)) == 0xDEADBEEF)
                    continue;

                for (int i = 0; i < 8; ++i) {
                    coord prev;
                    prev.x = curr.x + dirs[i][0];
                    prev.y = curr.y + dirs[i][1];
                    // if the coordinate to check is in bounds, check it
                    // moving from prev to this node
                    if ((0 <= prev.x && prev.x < graph.w) &&
                        (0 <= prev.y && prev.y < graph.h) /*&&
                        calc_cost(map_get(map, prev.x, prev.y)) != 0xDEADBEEF*/) {
                        uint32_t cost = 0;
                        cost += dirs[i][2];
                        cost += graph_node(&graph, prev.x, prev.y).cost;
                        cost += calc_cost(map_get(map, curr.x, curr.y)) << 1;

                        // redirect current node if it costs less
                        node * n = graph_ref(&graph, curr.x, curr.y);
                        if (cost < n->cost) {
                            run = 1;

                            dprintf("(%d,%d) found better path from (%d, %d); cost %d -> %d\n",
                                    curr.x, curr.y, prev.x, prev.y, n->cost, cost);
                            n->cost = cost;
                            n->dir_x = dirs[i][0];
                            n->dir_y = dirs[i][1];
                        }
                    }
                }
            }
        }

    } while (run && count < (graph.h * graph.w));
    dprintf("cycles: %d\n", count);

    // generate the path
    gen_path(&graph, start, end, path);
}

static const int acceldirs[8][2] =
{
    // x  ,  y,
    {DIR_H, DIR_U},
    {DIR_R, DIR_U},
    {DIR_R, DIR_H},
    {DIR_R, DIR_D},
    {DIR_H, DIR_D},
    {DIR_L, DIR_D},
    {DIR_L, DIR_H},
    {DIR_L, DIR_U},
};

coord path_play(path * path, const map * map, const char * path_path, const coord * end_p)
{
    FILE * f = fopen(path_path, "r");
    coord start;
    coord end = *end_p;
    fscanf(f, "%08x", &start.x);
    fscanf(f, "%08x", &start.y);
    printf("(%d,%d)\n", start.x, start.y);

    path_ctor(path);
    uint32_t val;
    int count = 0;
    // generate a graph from this
    graph graph;
    gen_graph(&graph, map);
    for (int r = 0; r < map->h; ++r) {
        for (int c = 0; c < map->w; ++c) {
            if (count == 0) {
                fscanf(f, "%08x", &val);
            }
            uint8_t dir = val & 0xF;
            val >>= 4;
            node * n = graph_ref(&graph, c, r);
            if (dir & 8) {
                dir = dir & 0x7;
                n->dir_x = acceldirs[dir][0];
                n->dir_y = acceldirs[dir][1];
            } else {
                n->dir_x = DIR_H;
                n->dir_y = DIR_H;
            }
            count = (count + 1) % 8;
        }
    }

    gen_path(&graph, &start, &end, path);

    return start;
}

void path_load(const map * map, const coord * start, const coord * end,
               const uint32_t * buffer, path * path)
{
    path_ctor(path);
    uint32_t val;
    int count = 0;
    // generate a graph from this
    graph graph;
    gen_graph(&graph, map);
    for (int r = 0; r < map->h; ++r) {
        for (int c = 0; c < map->w; ++c) {
            if (count == 0) {
                val = *buffer;
                ++buffer;
            }
            uint8_t dir = val & 0xF;
            val >>= 4;
            node * n = graph_ref(&graph, c, r);
            if (dir & 8) {
                dir = dir & 0x7;
                n->dir_x = acceldirs[dir][0];
                n->dir_y = acceldirs[dir][1];
            } else {
                n->dir_x = DIR_H;
                n->dir_y = DIR_H;
            }
            count = (count + 1) % 8;
        }
    }

    gen_path(&graph, start, end, path);
}
