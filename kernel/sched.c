#include "include/proc.h"
#include "include/defs.h"
#include "include/types.h"

extern struct proc procs[NPROC];

bool scheduler(uint64_t *f, uint64_t *mepc, uint64_t *satp) {
    struct proc *p;

    for (p = procs; p < &procs[NPROC]; p++) {
        if (p->state == RUNNING) {
            break;
        }
    }

    if (p == NULL) {
        return false;
    }

    *f = (uint64_t)p->frame;
    *mepc = (uint64_t)p->pc;
    *satp = (uint64_t)p->pgt;

    printf("scheduling %d\n", p->pid);

    if (*satp != 0) {
        *satp = ((uint64_t)8 << 60 | ((uint64_t)p->pid << 44) | *satp);
    }

    return true;
}