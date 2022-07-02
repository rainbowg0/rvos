#include "include/defs.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/memlayout.h"

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

// mark the start of the actual memory we can dish out.
static uint64_t _alloc_start = 0;
static uint64_t _alloc_end = 0;
static uint64_t _num_pages = 0;

typedef enum {
    empty = (1<<0),
    taken = (1<<1), // is current page allocated?
    last = (1<<2),  // is current page the last of a contiguous allocation?
} flag;

// Each page is described by the page structure.
// Each 4096-byte chunk of memory has a structure
// associated with it.
typedef struct page {
    flag f;
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
    _num_pages = HEAP_SIZE / PGSIZE;
    page *ptr = (page *)HEAP_START;
    // clear all pages
    for (int i = 0; i < _num_pages; i++) {
        _clear(ptr++);
    }

    // Determin where the actual useful memory start.
    // After all page structures. Also, align the ALLOC_START
    // to a page-boundary (PAGESIZE = 4096). 
    _alloc_end = PGROUNDUP(HEAP_START + _num_pages * sizeof(page));

    printf("TEXT:   0x%x -> 0x%x\n", TEXT_START, TEXT_END);
	printf("RODATA: 0x%x -> 0x%x\n", RODATA_START, RODATA_END);
	printf("DATA:   0x%x -> 0x%x\n", DATA_START, DATA_END);
	printf("BSS:    0x%x -> 0x%x\n", BSS_START, BSS_END);
	printf("HEAP:   0x%x -> 0x%x\n", _alloc_start, _alloc_end);
}

// Allocate pages
void *pagealloc(int np) {
    assert(np > 0);

    // reserved 8 page (8 * 4096) to hold the page structure
    // It should be allocated enough space to manage whole memory space of 128MB.
    _num_pages = (HEAP_SIZE / PGSIZE) - 8;
    page *ptr = (page *)HEAP_START;
    bool found = false;
    for (int i = 0; i < _num_pages - np; i++) {
        // allocate a free page
        if (_isfree((ptr + i))) {
            found = true;
            for (int j = i; j < i + np; j++) {
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
            _setflag((ptr+i+np-1), last);
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
    uint32_t size = (PGSIZE * np) / 8;
    uint64_t *ptr = (uint64_t*)ps;
    for (uint32_t i = 0; i < size; i++) {
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
	if (!p || (uint64_t)p >= _alloc_end) {
		return;
	}
	/* get the first page descriptor of this memory block */
	page *ptr = (page *)HEAP_START;
	ptr += ((uint64_t)p - _alloc_start) / PGSIZE;
	/* loop and clear all the page descriptors of the memory block */
	while (!_isfree(ptr)) {
		if (_islast(ptr)) {
			_clear(ptr);
			break;
		} else {
			_clear(ptr);
			ptr++;;
		}
	}
}

// Print all page allocations.
void printpages() {
    uint32_t np = HEAP_SIZE / PGSIZE;
    page *st = (page*)HEAP_START;
    page *ed = st + np;
    uint32_t alloc_ed = _alloc_start + np * PGSIZE;
    printf("\n");
    printf("PAGE ALLOCATION TABLE\nMETA: %p -> %p\nPHYS: %p -> %p\n", st, ed, _alloc_start, alloc_ed);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    uint32_t num = 0;
    while (st < ed) {
        if (_istaken(st)) {
            uint64_t memaddr = _alloc_start + ((uint64_t)st - HEAP_START) * PGSIZE;
            printf("%p =>", memaddr);
            while (1) {
                num += 1;
                if (_islast(st)) {
                    printf("%p %d\n", st, num);
                    break;
                }
            }
            st++;
        }
        st++;
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Allocated: %d pages %d bytes\n", num, num * PGSIZE);
    printf("Free     : %d pages %d bytes\n", _num_pages - num, (_num_pages - num) * PGSIZE);
    printf("\n");
}
