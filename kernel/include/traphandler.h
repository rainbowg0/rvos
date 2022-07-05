#ifndef RVOS_TRAPHANDLER_H
#define RVOS_TRAPHANDLER_H

#include "types.h"

struct trapframe{
    uint64_t regs[32];
    uint64_t fregs[32];
    uint64_t satp;
    uint8_t *trapstack;
    uint64_t hartid;
};

#endif //RVOS_TRAPHANDLER_H