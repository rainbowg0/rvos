#include "include/defs.h"
#include "include/memlayout.h"
#include "include/riscv.h"

static uint64_t KERNEL_TABLE;

//////////////////////////////////
// ENTRY POINT
//////////////////////////////////
uint64_t kinit() {
    // kinit() runs in machine mode
    // the job of kinit is to get us into supervisor mode
    // as soon as possible.
    uartinit();
    pageinit();
    kmeminit();

    //printpages();

    //pagetest();

    // map heap allocation
    pagetable_t kpagetable = gettable();
    uint64_t head = (uint64_t)gethead();
    uint64_t npages = getnumalloc();

    printf("TEXT:   0x%x -> 0x%x\n", TEXT_START, TEXT_END);
	printf("RODATA: 0x%x -> 0x%x\n", RODATA_START, RODATA_END);
	printf("DATA:   0x%x -> 0x%x\n", DATA_START, DATA_END);
	printf("BSS:    0x%x -> 0x%x\n", BSS_START, BSS_END);
    printf("STACK:  0x%x -> 0x%x\n", KERNEL_STACK_START, KERNEL_STACK_END);
	printf("HEAP:   0x%x -> 0x%x\n", head, head + npages * PGSIZE);

    maprange(kpagetable, head, head + (npages + 1) * 4096, PTE_R|PTE_W);

    // map heap descriptor
    uint64_t num_pages = HEAP_SIZE / PGSIZE;
    maprange(kpagetable, HEAP_START, HEAP_START + num_pages, PTE_R|PTE_W);

    // map executable section
    maprange(kpagetable, TEXT_START, TEXT_END, PTE_R|PTE_X);

    // map rodata section
    // put rodata section into the text section, so they can 
    // potentially overlap however.
    maprange(kpagetable, RODATA_START, RODATA_END, PTE_R|PTE_X);

    // map data section
    maprange(kpagetable, DATA_START, DATA_END, PTE_R|PTE_W);

    // map bss section
    maprange(kpagetable, BSS_START, BSS_END, PTE_R|PTE_W);

    // map kernel stack
    maprange(kpagetable, KERNEL_STACK_START, KERNEL_STACK_END, PTE_R|PTE_W);

    // UART
    pagemap(kpagetable, UART0, UART0, PTE_R|PTE_W, 0);

    // CLINT
    pagemap(kpagetable, CLINT, CLINT, PTE_R|PTE_W, 0);

    // MTIMECMP
    pagemap(kpagetable, 0x200b000, 0x200b000, PTE_R|PTE_W, 0);

    // MTIME
    pagemap(kpagetable, 0x200c000, 0x200c000, PTE_R|PTE_W, 0);

    // PLIC
    maprange(kpagetable, PLIC, 0xc002000, PTE_R|PTE_W);
    maprange(kpagetable, 0xc200000, 0xc208000, PTE_R|PTE_W);

    printpagealloc();

    //printf("\n");
    //printkmemtable();

    // the following shows how to walk to tanslate a virtual
    // address into a physical address, use this whenever a
    // user space application requires services. user space 
    // allocation only knows virtual addresses, we have to
    // translate silently behind the success.
    //uint64_t p = 0x80053000;
    //uint64_t m = va2pa(kpagetable, p);
    //printf("walk 0x%x -> 0x%x\n", p, m);

    // when return from here, we'll go back to entry.S and switch
    // into supervisor mode, we will return the SATP register
    // to be written when we return.
    // in SATP, the physical address of kpagetable should be divided 
    // by 4KB, and enable MMU by setting mode 8, bits[63:60] determin
    // 0 for Base(no translation)
    // 8 for Sv39
    // 9 for Sv48
    KERNEL_TABLE = (uint64_t)kpagetable;

    uint64_t satp_val = ((uint64_t)kpagetable >> 12) | ((uint64_t)8 << 60);
    w_satp(satp_val);

    return satp_val;
}

void kmain() {
    // kmain() starts in supervisor mode, so we should have the trap
    // vector setup and MMU turned on when get here.
    uartputs("hello, os world!\n");

    uint8_t *ptr = kzmalloc(16);
    for (int i = 0; i < 16; i++) {
        ptr[i] = i;
    }
    printf("%s\n", ptr);
    printkmemtable();
    kfree(ptr);


    while (1) {}
}
