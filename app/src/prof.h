#ifndef __PROF_H__
#define __PROF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <time.h>
#include <sys/types.h>

typedef struct prof_
{
    uint64_t    prproc;
    uint64_t    tx;
    uint64_t    exec;
    uint64_t    rx;
    uint64_t    poproc;
    struct timespec start;
    struct timespec end;
} prof;

static inline
prof_ctor(prof * prof)
{
    prof->prproc = 0;   // pre processing (e.g. graph generation)
    prof->tx     = 0;   // transmission time
    prof->exec   = 0;   // execution time
    prof->rx     = 0;   // receive time
    prof->poproc = 0;   // post processing (e.g. path generation)
}

static inline
void prof_start(prof * prof){clock_gettime(CLOCK_MONOTONIC, &prof->start);}

static inline
void prof_end(prof * prof){clock_gettime(CLOCK_MONOTONIC, &prof->end);}

static inline
uint64_t prof_dt(prof * prof)
{
    uint64_t t1 = (uint64_t) prof->end.tv_sec * 1000000000 + prof->end.tv_nsec;
    uint64_t t0 = (uint64_t) prof->start.tv_sec * 1000000000 + prof->start.tv_nsec;
    uint64_t dt = t1 - t0;
    return dt;
}

static inline
void prof_print(prof * prof)
{
    uint64_t total = prof->prproc +  prof->tx + prof->exec + prof->rx + prof->poproc;
    printf("Total time          : %llu ns\n", total);
    printf("Time to preprocess  : %llu ns (%0.2f%%)\n", prof->prproc,
           (float) prof->prproc / (float) total * 100.0f);
    printf("Time to transfer    : %llu ns (%0.2f%%)\n", prof->tx,
           (float) prof->tx / (float) total * 100.0f);
    printf("Time to execute     : %llu ns (%0.2f%%)\n", prof->exec,
           (float) prof->exec / (float) total * 100.0f);
    printf("Time to receive     : %llu ns (%0.2f%%)\n", prof->rx,
           (float) prof->rx / (float) total * 100.0f);
    printf("Time to postprocess : %llu ns (%0.2f%%)\n", prof->poproc,
           (float) prof->poproc / (float) total * 100.0f);
}

#ifdef __cplusplus
}
#endif

#endif//__PROF_H__
