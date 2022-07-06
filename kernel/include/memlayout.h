#ifndef RVOS_MEMLAYOUT_H
#define RVOS_MEMLAYOUT_H

#include "types.h"

// Physical memory layout

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0) // set the priority of a paticular interrupt source
#define PLIC_PENDING (PLIC + 0x1000) // contain a list of interrupts have been triggered
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100) // enable/disable certions interrupr sources
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100) 
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_THRESHOLD (PLIC + 0x200000) // set the threshold that interrupts must
                                         // meet before being able to trigger.)
// return the next interrupt in priority order
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)
#define PLIC_CLAIM (PLIC + 0x200004)
#define PLIC_INT_ENABLE (PLIC + 0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)


extern uint64_t TEXT_START;
extern uint64_t TEXT_END;
extern uint64_t DATA_START;
extern uint64_t DATA_END;
extern uint64_t RODATA_START;
extern uint64_t RODATA_END;
extern uint64_t BSS_START;
extern uint64_t BSS_END;
extern uint64_t HEAP_START;
extern uint64_t HEAP_SIZE;
extern uint64_t KERNEL_STACK_START;
extern uint64_t KERNEL_STACK_END;

#endif //RVOS_MEMLAYOUT_H