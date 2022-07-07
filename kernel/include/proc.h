#include "types.h"
#include "riscv.h"
#include "trap.h"
#include "param.h"
#include "spinlock.h"

// allocate to pages for stack
#define STACK_SIZE (8192)
// adjust the stack to be at the bottom of the memory allocation
// regardless of where it is on the kernel heap.
#define STACK_ADDR (0xf00000000)
// all processes will have a defined starting point in virtual memory
#define STARTING (0x20000000)

enum procstate { UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// the private data in a process contains information
// that is relevant to where we are, including the path
// and open file descriptors.
struct procdata {
    uint8_t cwd_path[128];
};

// the process contains a stack, which gives sp register.
// the stack shows the top of the stack, so need to add the
// PGSIZE, because stack grows from high -> low.
// pc contain the address of the instruction to be executed next.
// get through mepc register (Machine Exception Program Counter)
// when interrupt out process either with context-switch timer or
// an illegal instruction, or as a response to a UART input, the
// CPU will put the instruction about to be executed in mepc.
// we can restart out process by going back here and start
// executing instruction again.
struct proc {
    struct spinlock lock;

    struct trapframe *frame;
    uint8_t *stack;
    uint64_t pc;
    uint16_t pid;
    pagetable_t pgt;
    enum procstate state;
    struct procdata data;
};

// saved registers for kernel context switches
struct context {
    uint64_t ra;
    uint64_t sp;

    // callee-saved
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
};

// per-cpu state
struct cpu {
    struct proc *proc; // the process running on this cpu
    struct context context; // swtch() here to enter scheduler()
    int noff; // depth of push_off() nesting
    int intena; // were interrupts enabled before push_off()
};

extern struct cpu cpus[NCPU];
