#ifndef RVOS_TRAP_H
#define RVOS_TRAP_H

#include "types.h"

// process is defined by its context, the trapframe
// shows what it looks like on the CPU
// 1. the general purpose registers (32)
// 2. the floating point registers (32)
// 3. mmu
// 4. a stack for handling this process's context
struct trapframe {
    uint64_t regs[32];      // 0-255
    uint64_t fregs[32];     // 256-511
    uint64_t satp;          // 512-519
    uint8_t  *trapstack;    // 520
    uint64_t hartid;        // 528
};

#endif //RVOS_TRAP_H