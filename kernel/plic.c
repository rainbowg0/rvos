#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/types.h"

// each register of PLIC is 4-bytes
// the PLIC is an external interrupt controller.
// the one used by QEMU is the same as the SiFive PLIC.

// the virt machine have the following external interrupts:
// interrupt 0 is a 'null' and is hardwired to 0
// VIRTIO = [1..8]
// UART0 = 10
// PCIE (PCI express devices) = [32..35]

// get the next available interrupt. this is the 'claim' process.
// the plic will automatically sort by priority and hand us the
// ID of the interrupt, for example if the UART is interrupting
// and it's next, we will get the value 10.
uint32_t plic_next() {
    return RREG(PLIC_CLAIM);
}

// complete a pending interrupt by id. the id should come
// from the next() function above.
void plic_complete(uint32_t id) {
    // we actually write a uint32_t into the entire complete_register
    // this is the same register as the claim register, but it can
    // differentiate based on whether we're reading or writing.
    WREG(PLIC_CLAIM, id);
}

// set the global threshold. the threshold can be a value [0..7]
// the plic will mask any interrupts at or below the given threshold.
// this means that a threshold of 7 will mask ALL interrupts and
// a threshold of 0 will allow ALL interrupts.
void plic_setthreshold(uint8_t tsh) {
    // do tsh because use a u8, but maximum number is 3-bit 0b111
    // so and 0b1111 to get the last three bits.
    uint8_t actual_tsh = tsh & 7;
    WREG(PLIC_THRESHOLD, actual_tsh);
}

// see if a given interrupt id is pending
bool plic_ispending(uint32_t id) {
    uint32_t pend_ids = RREG(PLIC_PENDING);
    uint64_t actual_id = 1 << id;
    return actual_id & pend_ids;
}

// enable a given interrupt id
void plic_enable(uint32_t id) {
    uint32_t enables = RREG(PLIC_INT_ENABLE);
    WREG(PLIC_INT_ENABLE, enables | (1 << id));
}

// set a given interrupt priority to the given priority
// the priority must be [0..7]
void plic_setpriority(uint32_t id, uint8_t pri) {
    uint32_t actual_pri = (uint32_t)pri & 7;
    uint32_t *pri_reg = (uint32_t*)PLIC_PRIORITY;
    pri_reg += id;
    *pri_reg = actual_pri;
}
