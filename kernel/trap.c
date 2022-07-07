#include "include/trap.h"
#include "include/riscv.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/defs.h"

void external_interrupt() {
    // machine external (interrupt from PLIC).
    // check the next interrupt, if the interrupt isn't vailable,
    // get zere, however, that would mean we got a suprious interupt,
    // unless get an interrupt from non-PLIC source. this is the main
    // reason that the PLIC hardwires the id 0 to 0. so that use is as an error.
    uint32_t interrupt = plic_next();
    if (interrupt == 0) {
        return;
    }
    int val = 0;
    switch (interrupt) {
    // got an interrupt from the claim register, the PLIC will automatically
    // prioritize the next interrupt, so when get from claim, it will
    // be the next in priority order.
    case 10:
        // interrupt 10 is the UART interrupt.
        // we would typically set this to be handled out of the interrupt context.
        // but we're testing here.
        val = uartgetc();
        if (val == -1) {
            break;
        }
        switch (val)
        {
        case 8:
            // this is backspace, so write a space and backup again.
            printf("%c %c", (char)(val), (char)(val));
            break;
        case 10 | 13:
            // newline or carriage-return
            printf("\n");
            break;
        default:
            printf("%c", (char)(val));
            break;
        }
        break;
    default:
        // non-UART interrupts go here and do nothing
        printf("Non-UART external interrupt: %d\n", interrupt);
        break;
    }
    // We've claimed it, so now say that we've handled it. This resets the interrupt pending
	// and allows the UART to interrupt again. Otherwise, the UART will get "stuck".
    plic_complete(interrupt);
}

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
            printf("timer interrupt\n");
            *(uint32_t*)CLINT_MTIMECMP(hart) = *(uint32_t*)CLINT_MTIME + 10000000;
            break;
        case 11:
            external_interrupt();
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
            ret_pc += do_syscall(ret_pc, frame);
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