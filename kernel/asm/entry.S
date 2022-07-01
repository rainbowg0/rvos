# qemu load kernel at address 0x80000_0000

# with a 4096-byte stack per CPU.
.equ STACK_SIZE, 4096

.section .text
_entry:
    # csrr: 'control status register read', read hart id to a1
    csrr t0, mhartid
    mv tp, t0                   # keep current hartid in tp for later usage
    bnez t0, spin # if not hart0, spin

    # SATP should be zero, this can help us to control MMU.
    # since haven't set virtual memory, disable SATP by writing
    # zero into it with csrw (control status register write).
    csrw satp, zero

    # set up stacks, because stack grows from bottom to top,
    # put stack pointer to the end of stack.
    slli t0, t0, 12             # left shift the hart id by 4096
    la sp, stacks + STACK_SIZE  # sp = stacks + (hartid * 4096)
    add sp, sp, t0

    call main                   # hart0 jump to C

spin:
    wfi
    j spin

stacks:
    .skip STACK_SIZE * 8        # alocate stack space for 8 harts

    .end # end of file