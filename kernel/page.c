#include "include/defs.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/memlayout.h"

// mark the start of the actual memory we can dish out.
static uint64_t _alloc_start = 0;
static uint64_t num_pages;

typedef enum {
    empty = 0,
    taken = (1<<0), // is current page allocated?
    last = (1<<1),  // is current page the last of a contiguous allocation?
} flag;

// Each page is described by the page structure.
// Each 4096-byte chunk of memory has a structure
// associated with it.
typedef struct page {
    uint8_t f;
} page;

bool _islast(page *p) {
    if (p->f & last) {
        return true;
    }
    return false;
}

bool _istaken(page *p) {
    if (p->f & taken) {
        return true;
    }
    return false;
}

bool _isfree(page *p) {
    return !_istaken(p);
}

void _setflag(page *p, flag f) {
    p->f |= f;
}

void _clear(page *p) {
    p->f = empty;
}

// Initialize the allocation system. There are serval
// ways that we can implement the page allocater:
// 1. free list (singly linked list where it starts at the first free allocation)
// 2. bookkeeping list (structure contains a taken and length)
// 3. allocate on page structure per 4096 bytes. (V)
void pageinit() {
    num_pages = (HEAP_SIZE / PGSIZE) - 8;
    printf("HEAP_START = 0x%x, HEAP_SIZE = 0x%x, num of pages = %d\n",
        HEAP_START, HEAP_SIZE, num_pages);

    page *ptr = (page *)HEAP_START;
    // clear all pages
    for (int i = 0; i < num_pages; i++) {
        _clear(ptr++);
    }

    // Determin where the actual useful memory start.
    // After all page structures. Also, align the ALLOC_START
    // to a page-boundary (PAGESIZE = 4096). 
    _alloc_start = PGROUNDUP(HEAP_START + 8 * PGSIZE);

    printf("page init...\n");
}

// Allocate pages
void *pagealloc(int np) {
    assert(np > 0);

    // reserved 8 page (8 * 4096) to hold the page structure
    // It should be allocated enough space to manage whole memory space of 128MB.
    page *ptr = (page *)HEAP_START;
    bool found = false;
    for (int i = 0; i < num_pages - np; i++) {
        // allocate a free page
        if (_isfree((ptr + i))) {
            found = true;
            for (int j = i; j <= i + np; j++) {
                // check if there're contiguous allocation.
                // if not, check somewhere else;
                if (_istaken((ptr + j)))  {
                    found = false;
                    break;
                }
            }
        }

        // here have checken whether there are enough contiguous pages
        // to form what we need.
        if (found) {
            for (int k = i; k < i + np - 1; k++) {
                _setflag((ptr + k), taken);
            }
            _setflag((ptr+(i+np-1)), taken);
            _setflag((ptr+(i+np-1)), last);
            return (void*)(_alloc_start + i * PGSIZE);
        }
    }

    return NULL;
}

// Allocate and zero pages.
void *pagezalloc(int np) {
    void *ps = pagealloc(np);
    if (ps == NULL) {
        return NULL;
    }

    // get the raw allocation.
    uint64_t size = (PGSIZE * np) / 8;
    uint64_t *ptr = (uint64_t*)ps;
    for (uint64_t i = 0; i < size; i++) {
        // the reason to use uint64_t ptr is force sd(store doubleword).
        // rather than sb. This means 8x fewer stores than before.
        // Typically we have to be concerned about remaining bytes, but
        // fortunately 4096 % 8 == 0, so won't have any remaining bytes.
        ptr[i] = 0;
    }

    return ps;
}

// Deallocate a page.
// It will automatically colesce contiguous pages.
void pagedealloc(page *p) {
    /*
	 * Assert (TBD) if p is invalid
	 */
	if (p == NULL || (uint64_t)p >= (HEAP_START + HEAP_SIZE)) {
		panic("pagedealloc: dealloc a page that is out of range");
	}
	/* get the first page descriptor of this memory block */
	page *ptr = (page *)HEAP_START;
	ptr += ((uint64_t)p - _alloc_start) / PGSIZE;
	/* loop and clear all the page descriptors of the memory block */
	while (_istaken(ptr) && !_islast(ptr)) {
        _clear(ptr);
        ptr++;
    }
    if (!_islast(ptr)) {
        panic("pagedealloc: free a page out of range!");
    }

    _clear(ptr);
}

// Print all page allocations.
void printpagealloc() {
    page *st = (page*)HEAP_START;
    page *ed = st + num_pages;
    uint64_t alloc_ed = _alloc_start + num_pages * PGSIZE;
    printf("\n");
    printf("PAGE ALLOCATION TABLE\nMETA: 0x%x -> 0x%x\nPHYS: 0x%x -> 0x%x\n", \
            (uint64_t)st, (uint64_t)ed, \
            _alloc_start, alloc_ed);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    uint32_t num = 0;
    uint32_t cnt = 0;
    while (st < ed) {
        if (_istaken(st)) {
            uint64_t memaddr = _alloc_start + (st - (page*)HEAP_START) * PGSIZE;
            printf("0x%x => ", memaddr);
            while (1) {
                num += 1;
                cnt += 1;
                if (_islast(st)) {
                    uint64_t mem = _alloc_start + \
                            ((st) - (page*)HEAP_START) * PGSIZE + PGSIZE - 1;
                    printf("0x%x %d\n", mem, cnt);
                    cnt = 0;
                    break;
                }
                st++;
            }
        }
        st++;
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Allocated: %d pages %d bytes\n", num, num * PGSIZE);
    printf("Free     : %d pages %d bytes\n", num_pages - num, (num_pages - num) * PGSIZE);
    printf("\n");
}

// Paging is a system by which a piece of hardware(MMU) translates virtual addresses into
// physical addresses. The translation is performed by reading a priviledged register called
// SATP (Supervisor Address Translation and Protection Register), satp turns MMU on/off, sets
// the address space identifier, and sets the physical memory address where the first page
// table can be found.
//
// In satp, last 12 bit is not provided, we can take 44 bits from satp, and shift left by 12,
// replace last 12 bits with 0, so the 56-bit address is the result.
//
// Memory management have different modes, one is Sv39, where virtual address are 39-bits.
// 38     30 29    21 20    12 11           0
// +--------+––––––––+–––-----+-------------+
// | VPN[2] | VPN[1] | VPN[0] | page offset |
// +--------+--------+--------+-------------+
// each VPN(virtual page number) have 9-bits indices, where 2^9 = 512, so each table have 
// 512 entries and each entry have 8-bytes, to get physical address through virtual address
// it will iterate three-level page-tables to get the target page's physical address,
// and add the page offset can get the target byte in physical address.
// 
// Table Entries: written by operating system to control how the MMU works. We can change how
// an address is translated or set certain bits to protect a page:
// 63       54 53    28 27    19 18    10 9   8 7      0
// +----------+--------+--------+--------+-----+-------+
// | reserved | PPN[2] | PPN[1] | PPN[0] | RSW | flags |
// +----------+--------+--------+--------+-----+-------+
//
// Physical Addresses: is actually 56-bits. a 39-bit virtual address can translate into 56-bit
// physical address. Physical addresses is formed by taking PPN[2:0] and placing then into
// following format.
// 55     30 29    21 20    12 11           0
// +----------------------------------------+
// | PPN[2] | PPN[1] | PPN[0] | page offset |
// +----------------------------------------+
//
// SATP register: all translation begin at SATP, shown below:
// 63   60 59  44 43   0
// +-------------------+
// + MODE | ASID | PPN |
// +-------------------+
// 
// Three different kinds of page faults:
// 1. load (mcause/scause == 13)
// 2. store (mcause/scause == 15)
// 3. instruction (mcause/scause == 12)
//
// Translating Virtual Address:
// 1. read SATP and find top of level2's page table(PPN << 12).
// 2. add offset * size, offset = VPN[2] = 502 * 8 = SATP + 4016
// 3. read entry from page table.
// 4. check valid flag.
// 5. if R|W|X, get leaf.
// 6. if not get leaf, fetch next page from PPN[2:0], repeat until get leaf.
// If the page table entry(PTE) is a leaf, then PPN[2:0] describe the physical address.
// If PTE is a branch, the PPN[2:0] describe where the next page table can be found in
// physical RAM. Also, only PPN[2:leaf-level] will be used to develop the physical
// address. e.g. if level2's page table entry is leaf, then only PPN[2] contributed
// to physical address, VPN[1:0] is copied to PPN[1:0].

bool _isvalid(pte_t e) {
    return e & PTE_V;
}

bool _isinvalid(pte_t e) {
    return !_isvalid(e);
}

bool _isleaf(pte_t e) {
    return e & 0xe;
}

// map() function takes a mutable root by-reference, a virtual address, a physical
// address, the protection bits, and which level this hsould be mapped to.
// - level 0, 4KB level.
// - level 1, 2MB level.
// - level 2, 1GB level.
void pagemap(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t bits, uint64_t level) {
    // make sure that 'r', 'w', or 'x' have been privided
    // otherwise, we'll leak memory and always create a page fault.
    if (!_isleaf(bits)) {
        panic("map: should set R|W|E");
    }

    va = PGROUNDDOWN(va);

    // just like the virtual address, extract the physical address number(PPN).
    // however, PPN[2] is different in that it stores 26 bits instead of 9.
    uint64_t ppn0 = (pa >> 12) & 0x1ff; // PPN[0] = paddr[20:12]
    uint64_t ppn1 = (pa >> 21) & 0x1ff; // PPN[1] = paddr[29:21]
    uint64_t ppn2 = (pa >> 30) & 0x3ffffff; // PPN[2] = paddr[55:30];

    pte_t *pte = &pagetable[PX(2, va)];
    int i;
    for (i = 2; i > level; i--) {
        pte = &pagetable[PX(i, va)];
        if (*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            page *p = pagezalloc(1);
            if (p == NULL) {
                panic("map: no free physical page left");
            }
            *pte = PA2PTE((uint64_t)p) | PTE_V;
            pagetable = (pagetable_t)PTE2PA(*pte);
            //printf("alloc pd 0x%x, pte2pa 0x%x\n", p, PTE2PA(*pte));
        }
    }

    // get to the leaf page
    pte = &pagetable[PX(0, va)];
    *pte = (ppn2 << 28) | (ppn1 << 19) | (ppn0 << 10) | bits | PTE_V;
    //*pte = PA2PTE(pa) | bits | PTE_V;
    printf("%d: map va 0x%x to pa 0x%x, actual 0x%x, pte 0x%x\n", 
            i, va, pa, va2pa(pagetable, va), *pte);
}

// umap(): unmap and free all memory associated with a table.
// don't free root pagetable, for it's usually embedden into process structure.
void pageumap(pagetable_t pagetable) {
    for (int i = 0; i < 512; i++) {
        pte_t pte1 = pagetable[i];
        if ((pte1 & PTE_V) && (!_isleaf(pte1))) {
            // this is a valid entry, so drill down and free
            uint64_t child = PTE2PA(pte1);
            pagetable_t childtable = (pagetable_t)child;
            for (int j = 0; j < 512; j++) {
                pte_t pte2 = childtable[j];
                if ((pte2 & PTE_V) && (!_isleaf(pte2))) {
                    uint64_t pa = PTE2PA(pte2);
                    // the next level is level 0, free here.
                    pagedealloc((page*)pa);
                }
            }
            pagedealloc((page*)child);
        }
    }
}

// walk the page to convert a virtual address to a physical address.
// if a page fault would occur, return none.
// otherwise, return with the physical address.
uint64_t va2pa(pagetable_t pagetable, uint64_t va) {
    for (int i = 2; i >= 0; i--) {
        pte_t pte = pagetable[PX(i, va)];
        if (!(pte & PTE_V)) {
            // this is an invalid entry, page fault.
            break;
        } else if (_isleaf(pte)) {
            // a leaf can be at any level.
            // offset mask masks off the PPN, each PPn is 9 bits
            // and start at bit 12.
            uint64_t off_mask = (1 << (12 + i * 9)) - 1;
            uint64_t vaddr_pgoff = va & off_mask;
            return PTE2PA(pte) | vaddr_pgoff;
        }
        uint64_t child = PTE2PA(pte);
        pagetable = (pagetable_t)child;
    }

    return 0;
}

uint64_t getallocstart() {
    return _alloc_start;
}
