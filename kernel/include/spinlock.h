#ifndef RVOS_SPINLOCK_H
#define RVOS_SPINLOCK_H

#include "types.h"

struct spinlock {
    bool locked;    
    struct cpu *cpu;
};

#endif // RVOS_SPINLOCK_H