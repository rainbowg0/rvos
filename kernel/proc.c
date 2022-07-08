#include "include/proc.h"
#include "include/defs.h"
#include "include/spinlock.h"

// store a process list, uses the global allocator that made before
// and its job is to store all processes. we will have this list OWN
// the process. so anytime want the process, consult the process list.
struct proc procs[NPROC];
struct cpu cpus[NCPU];

extern void switch_to_user(uint64_t frame, uint64_t mepc, uint64_t satp);

extern uint64_t make_syscall(uint64_t);

// search through the process list to get a new PID, but
// it's probably easier and faster to increase the pid.
uint16_t next_pid = 1;
struct spinlock pid_lock;

// eventually move this function out of here, but its
// job is just to take a slot in the process list.
void initcode() {
    uint64_t i = 0;
    while (1) {
        i += 1;
        if (i > 70000000) {
            make_syscall(1);
            i = 0;
        }
    }    
 }

uint64_t proc_init() {
    struct proc *p;
    spin_init(&pid_lock);
    for (p = procs; p < &procs[NPROC]; p++) {
        spin_init(&p->lock);
    }

    p = proc_alloc(initcode);
    if (p == NULL) {
        panic("can't alloc new proc");
    }
    struct trapframe *frame = p->frame;
    pagetable_t pgt = p->pgt;
    w_mscratch((uint64_t)frame);
    w_satp(build_satp((uint64_t)8, (uint64_t)1, (uint64_t)pgt));
    // sfence_vma();
    printf("satp %p\n", r_satp());
    printf("frame address %p\n", frame);
    // synchronize PID 1, use ASID as the PID
    satp_fence_asid(1);

    printf("return 0x%x\n", p->pc);
    return p->pc;
 }

uint16_t proc_allocpid() {
    int pid;

    spin_acquire(&pid_lock);
    pid = next_pid;
    next_pid++;
    spin_release(&pid_lock);

    return pid;
}

// look in the process table for an UNUSED proc.
// if found, initialize state required to run in the kernel,
// and return with p->lock held.
// if there are no free proces, or a memory allocation fails, return 0.
struct proc* proc_alloc(void* fn) {
    uint64_t fn_pa = (uint64_t)fn;
    uint64_t fn_va = fn_pa;

    struct proc *p;
    for (p = procs; p < &procs[NPROC]; p++) {
        spin_acquire(&p->lock);
        if (p->state == UNUSED) {
            goto found;
        } else {
            spin_release(&p->lock);
        }
    }
    return NULL;

found:
    p->pid = proc_allocpid();

    if ((p->frame = (struct trapframe*)(pagezalloc(1))) == 0) {
        spin_release(&p->lock);
        return NULL;
    }
    if ((p->stack = (uint8_t*)pagezalloc(2)) == 0) {
        spin_release(&p->lock);
        return NULL;
    }
    p->pc = fn_va;
    if ((p->pgt = (uint64_t*)pagezalloc(1)) == 0) {
        spin_release(&p->lock);
        return NULL;
    }
    p->state = RUNNING;
    //struct procdata data;
    //p->data = data;

    // move the stack pointer to the bottom of the allocation.
    // the sepc shows that register x2(2) is the stack pointer.
    // also need to set the stack adjustment so that it is at
    // the bottom of the memory and far away from heap allocations.
    p->frame->regs[2] = STACK_ADDR + PGSIZE * 2;
    printf("sp %p\n", p->frame->regs[2]);
    pagetable_t pgt = p->pgt;
    uint64_t pa = (uint64_t)p->stack;
    // map the stack onto the user's virtual address. 
    for (int i = 0; i < 2; i++) {
        uint64_t off = i * PGSIZE;
        pagemap(pgt, STACK_ADDR + off, pa + off, PTE_R|PTE_W|PTE_U, 0);
    }

    for (int i = 0; i < 100; i++) {
        uint64_t modifier = i * 0x1000;
        printf("fn");
        pagemap(pgt, fn_va + modifier, fn_pa + modifier, PTE_R|PTE_W|PTE_X|PTE_U, 0);
    }

    // map the program counter on the MMU
    //pagemap(pgt, STARTING, (uint64_t)fn, PTE_R|PTE_X|PTE_U, 0);
    //pagemap(pgt, STARTING + 0x1001, (uint64_t)fn + 0x1001, PTE_R|PTE_X|PTE_U, 0);
    printf("fn addr: %p\n", (uint64_t)fn);
    printf("switch_to_user: 0x%x\n", (uint64_t)switch_to_user);
    pagemap(pgt, 0x80000000, 0x80000000, PTE_U|PTE_R|PTE_X, 0);

    printf("pagetable 0x%x\n", (uint64_t)pgt);
    printf("walk 0x%x -> 0x%x\n", (uint64_t)fn, va2pa(pgt, (uint64_t)fn));
    /*
    printf("STACK_ADDR: walk %p -> 0x%x\n", STACK_ADDR, va2pa(pgt, STACK_ADDR));
    printf("            walk %p -> 0x%x\n", STACK_ADDR + PGSIZE, va2pa(pgt, STACK_ADDR + PGSIZE));
    printf("STARTING:   walk 0x%x -> 0x%x\n", STARTING, va2pa(pgt, STARTING));
    printf("            walk 0x%x -> 0x%x\n", STARTING + 0x1001, va2pa(pgt, STARTING + 0x1001));
    */

    spin_release(&p->lock);
    return p;
}

// must be called with interrupts disabled
// to prevent race with process being moved
// to a different CPU.
uint32_t cpuid() {
    int id = r_tp();
    return id;
}

// return this CPU's cpu struct
// interrupts must be disabled
struct cpu* mycpu() {
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}
