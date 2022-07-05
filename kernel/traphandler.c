#include "include/traphandler.h"
#include "include/riscv.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/defs.h"

uint64_t m_trap(uint64_t epc, uint64_t tval, uint64_t cause, uint64_t hart, 
                uint64_t status, struct trapframe *frame) {
    // handle all traps in machine mode. RISC-V lets us delegate
    // to supervisor mode, but switching out SATP (virtual memory)
    // get hairy.
    bool is_async = (cause & ASYNC_BIT);

    // the cause contains the type of trap (sync, async) as well
    // as the cause number. so narrow down just the cause number
    uint64_t cause_num = cause & 0xfff;
    uint64_t ret_pc = epc;
    if (is_async) {
        switch (cause_num)
        {
        case 3:
            printf("Machine software interrupt CPU%d\n", hart);
            break;
        case 7:
            *(uint64_t*)CLINT_MTIMECMP(hart) = *(uint64_t*)CLINT_MTIME + 10000000;
            break;
        case 11:
            printf("Machine external interrupt CPU%d\n", hart);
            break;
        default:
            panic("Unhandled async trap CPU%d -> cause 0x%x\n", hart, cause);
            break;
        }
    } else {
        switch (cause_num)
        {
        case 2:
            panic("Illegal instruction CPU%d -> 0x%x, 0x%x, cause 0x%x\n", hart, epc, tval, cause);
            break;
        case 8:
            printf("Ecall from user mode! CPU%d ->0x%x\n", hart, epc);
            ret_pc += 4;
            break;
        case 9:
            printf("Ecall from supervisor mode! CPU%d -> 0x%x\n", hart, epc);
            break;
        case 11:
            panic("Ecall from machine mode! CPU%d ->0x%x\n", hart, epc);
            break;
        case 12:
            printf("Instruction page fault CPU%d -> 0x%x: 0x%x\n", hart, epc, tval);
            ret_pc += 4;
            break;
        case 13:
            printf("Load page fault CPU%d -> 0x%x: 0x%x\n", hart, epc, tval);
            ret_pc += 4;
            break;
        case 15:
            printf("Store page fault CPU%d -> 0x%x: 0x%x\n", hart, epc, tval);
            ret_pc += 4;
            break;
        default:
            panic("Unhandled sync trap CPU%d -> %d\n", hart, cause_num);
            break;
        }
    }

    return ret_pc;
}