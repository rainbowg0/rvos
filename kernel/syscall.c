#include "include/trap.h"
#include "include/types.h"
#include "include/defs.h"

uint64_t do_syscall(uint64_t mepc, struct trapframe *frame) {
    uint64_t sysno = frame->regs[10];
    switch (sysno)
    {
    case 0:
        mepc += 4;
        break;
    case 1:
        printf("test syscall\n");
        mepc += 4;
    
    default:
        printf("unknown syscall number %d\n", sysno);
        break;
    }

    return mepc;
}