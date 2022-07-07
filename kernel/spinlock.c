#include "include/spinlock.h"
#include "include/defs.h"
#include "include/riscv.h"
#include "include/proc.h"


bool holding(struct spinlock *lk) {
    return lk->locked && lk->cpu == mycpu();
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes tow pop_off() to undo two push_off(), also, if interrupts
// are initially off, then push_off, pop_off leaves them off.
void push_off() {
    int old = intr_get();

    struct cpu *c = mycpu();

    intr_off();
    if (c->noff == 0) {
        c->intena = old;
    }
    c->noff += 1;
}

void pop_off() {
    struct cpu *c = mycpu();
    if (intr_get()) {
        panic("pop_off: interruptiable");
    }
    if (c->noff < 1) {
        panic("pop_off");
    }
    c->noff -= 1;
    if (c->noff == 0 && c->intena) {
        intr_on();
    }
}

void spin_init(struct spinlock *lk) {
    lk->locked = false;
    lk->cpu = NULL;
}

// acquire the lock
// loops until the lock is acquired
void spin_acquire(struct spinlock *lk) {
    push_off(); // disable interrupts to avoid deadlock
    if (holding(lk)) {
        panic("acquire");
    }

    // on RISC-V, sync_lock_test_and_set turns into an atomic swap:
    // a5 = 1
    // s1 = &lk->locked
    // amoswap.w.aq a5, a5, (s1)
    while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;

    // tell the C ompiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // on RISC-V, this emits a fence instruction.
    __sync_synchronize();

    lk->cpu = mycpu();
}

// release the lock
void spin_release(struct spinlock *lk) {
    if (!holding(lk)) {
        panic("release");
    }

    lk->cpu = NULL;

    // tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the cirtical section occur strictly before the
    // lock is released.
    // on RISC-V, this emits a fence instruction.
    __sync_synchronize();

    // release the lock, equivalent to lk->locked = 0
    // this code doesn't use a C assignment, since the c standard
    // implies that an assignment might be implemented with
    // mutiple store instructions.
    // on RISC-V, sync_lock_release turns into an atomic swap:
    // s1 = &lk->locked
    // amoswap.w zero, zero, (s1)
    __sync_lock_release(&lk->locked);

    pop_off();
}
