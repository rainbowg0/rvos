#include "include/defs.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/trap.h"

//static uint64_t KERNEL_TABLE;
static struct trapframe trapframes[8];

//////////////////////////////////
// ENTRY POINT
//////////////////////////////////
uint64_t kinit() {
    /*
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

    maprange(kpagetable, head, head + (npages) * 4096, PTE_R|PTE_W);

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
    pagemap(kpagetable, CLINT_MTIMECMP(0), CLINT_MTIMECMP(0), PTE_R|PTE_W, 0);

    // MTIME
    pagemap(kpagetable, CLINT_MTIME, CLINT_MTIME, PTE_R|PTE_W, 0);

    // PLIC
    maprange(kpagetable, PLIC, 0xc002001, PTE_R|PTE_W);
    maprange(kpagetable, 0xc200000, 0xc208001, PTE_R|PTE_W);

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

    uint64_t satp_val = build_satp(8, 0, KERNEL_TABLE);

    // have to store kernel's table, the tables will be moved back
    // adn forth between kernel's table and user applications' table
    w_mscratch((uint64_t)&trapframes[0]);
    w_sscratch(r_mscratch());
    trapframes[0].satp = satp_val;

    // move the stack pointer to the very bottom. the stack is
    // actually in a non-mapped page. the stack is decrement-before
    // push and increment after pop. therefore, the stack will be 
    // allocated before it is stored.
    trapframes[0].trapstack = (uint8_t*)((uint64_t)pagezalloc(1) + PGSIZE);
    maprange(kpagetable, (uint64_t)trapframes[0].trapstack - PGSIZE, \
            (uint64_t)(trapframes[0].trapstack), PTE_R|PTE_W);

    // the trap frame itself is stored in the mcratch register
    maprange(kpagetable, r_mscratch(), r_mscratch() + sizeof(struct trapframe) * 8, PTE_R|PTE_W);

    printpagealloc();
    //uint64_t p = (uint64_t)trapframes[0].trapstack - 1;
    //printf("walk 0x%x -> 0x%x\n", p, va2pa(kpagetable, p));

    //p = CLINT_MTIMECMP(0);
    //printf("walk 0x%x -> 0x%x\n", p, va2pa(kpagetable, p));

    // the following shows how we're going to walk to translate a virtual
    // address into a physical address. we will use this whenever a user
    // space application requires services. since the user space application
    // only knows virtual address, we have to translate silently behind
    // the success.
    printf("setting %p\n", satp_val);
    printf("scratch reg = 0x%x\n", r_mscratch());
    w_satp(satp_val);
    satp_fence_asid(0);

    */
    // kinit() runs in machine mode
    // the job of kinit is to get us into supervisor mode
    // as soon as possible.
    uartinit();
    pageinit();
    kmeminit();
    uint64_t addr = proc_init();
    printf("init process created at address 0x%x\n", addr);
    
    printf("TEXT:   0x%x -> 0x%x\n", TEXT_START, TEXT_END);
	printf("RODATA: 0x%x -> 0x%x\n", RODATA_START, RODATA_END);
	printf("DATA:   0x%x -> 0x%x\n", DATA_START, DATA_END);
	printf("BSS:    0x%x -> 0x%x\n", BSS_START, BSS_END);
    printf("STACK:  0x%x -> 0x%x\n", KERNEL_STACK_START, KERNEL_STACK_END);

    plic_setthreshold(0);
    // virtio = [1..8]
    // uart0 = 10
    // pcie = [32..35]
    // enable uart interrupt
    plic_enable(10);
    plic_setpriority(10, 1);
    printf("UART interrupts have been enabled and are awaiting command\n");
    printf("Getting ready for first precess.\n");
    printf("issuing the first context switch timer\n");
    *(uint64_t*)CLINT_MTIMECMP(0) = *(uint64_t*)CLINT_MTIME + 10000000;

    printf("satp %p\n", r_satp());
    pagetable_t pgt = (pagetable_t)0x8021c000;
    printf("walk addr %p -> 0x%x\n", addr, va2pa(pgt, addr));

    maprange(pgt, KERNEL_STACK_START, KERNEL_STACK_END, PTE_R|PTE_W|PTE_U);
    return addr;
}

void kinit_hart(uint64_t hartid) {
    // all non-zero harts initialize here
    // we have to store the kernel's table. the table will be
    // moved back and forth between the kernel's table and
    // user applications' tables.
    w_mscratch((uint64_t)(&trapframes[hartid]));
    // copy the same mscratch over to the supervisor version of 
    // the same register
    w_sscratch(r_mscratch());
    trapframes[hartid].hartid = hartid;

    //*(uint64_t*)CLINT_MTIMECMP(hartid) = *(uint64_t*)CLINT_MTIME + 10000000;
    printf("hart%d bool\n", hartid);
}

void kmain() {
    // kmain() starts in supervisor mode, so we should have the trap
    // vector setup and MMU turned on when get here.
    printf("hello, os world\n");
    //printf("%d: hello, os world!\n", r_mhartid());

    // set the next machine timer to fire
    *(uint64_t*)CLINT_MTIMECMP(0) = *(uint64_t*)CLINT_MTIME + 10000000;

    // try to cause a page fault
    //uint64_t *v = 0;
    //*v = 1;

    // set the interrupt system via the PLIC, have to set the threshold to
    // something that won't mask all interrupts.
    printf("Setting up interrupts and PLIC...\n");
    // lower the threshold wall so all interrupts can jump over it.
    plic_setthreshold(0);
    // virtio = [1..8]
    // uart0 = 10
    // pcie = [32..35]
    // enable uart interrupt
    plic_enable(10);
    plic_setpriority(10, 1);
    printf("UART interrupts have been enabled\n");
   
    while (1) {}
}
