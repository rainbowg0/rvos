#ifndef RVOS_MEMLAYOUT_H
#define RVOS_MEMLAYOUT_H

/*****************************************
 * physical address memlayout, including:
 * - UART
 * - VIRTIO
 * - CLINT
 * - PLIC
 ****************************************/

// UART's base address
#define UART0 0x10000000L // 0x1000_0000
#define UART0_IRQ 10

#define KERNBASE 0x80000000L // 0x8000_0000

#endif //RVOS_MEMLAYOUT_H