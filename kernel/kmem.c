#include "include/defs.h"
#include "include/types.h"
#include "include/riscv.h"

// in the future, we will have on-demand pages
// so, we need to keep track of our memory footprint to
// see if we actually need to allocate more.
typedef struct alloclist {
    uint64_t flags_size;
} alloclist;

static alloclist* KMEM_HEAD;
static uint64_t KMEM_ALLOC;
static pagetable_t KMEM_PAGE_TABLE;

struct alloclist *gethead() {
    return KMEM_HEAD;
}

uint64_t getnumalloc() {
    return KMEM_ALLOC;
}

pagetable_t gettable() {
    return KMEM_PAGE_TABLE;
}

bool _alistaken(alloclist *a) {
    return a->flags_size & AllocFlag;
}

bool _alisfree(alloclist *a) {
    return !_alistaken(a);
}

void _alsettaken(alloclist *a) {
    a->flags_size |= AllocFlag;
}

void _alsetfree(alloclist *a) {
    a->flags_size &= ~AllocFlag;
}

void _alsetsize(alloclist *a, uint64_t sz) {
    bool k = _alistaken(a);
    a->flags_size = sz & ~AllocFlag;
    if (k) {
        a->flags_size |= AllocFlag;
    }
}

uint64_t _algetsize(alloclist *a) {
    return a->flags_size & ~AllocFlag;
}

// initialize kernel's memory
// this is not to be used to allocate memory for user processes.
// if that's the case, use alloc/dealloc from page.c
void kmeminit() {
    // allocate 64 kernel pages (64 * 4096 = 262KB)
    struct page *p = pagezalloc(512);
    if (p == NULL) {
        panic("kemeinit: no free memory");
    }

    KMEM_ALLOC = 512;
    KMEM_HEAD = (alloclist*)p;
    _alsetfree(KMEM_HEAD);
    _alsetsize(KMEM_HEAD, KMEM_ALLOC * PGSIZE);
    KMEM_PAGE_TABLE = (pagetable_t)pagezalloc(1);
}

// allocate sub-page level allocation based on bytes
uint8_t *kmalloc(uint64_t sz) {
    uint64_t o = (1 << 3) - 1;
    sz = (sz + o) & !o;
    uint64_t size = sz + sizeof(alloclist);
    alloclist* head = KMEM_HEAD;
    alloclist* tail = (alloclist*)((uint8_t*)KMEM_HEAD + (KMEM_ALLOC * PGSIZE));

    while (head < tail) {
        if (_alisfree(head) && size <= _algetsize(head)) {
            uint64_t chunk_size = _algetsize(head);
            uint64_t rem = chunk_size - size;
            _alsettaken(head);
            if (rem >sizeof(alloclist)) {
                alloclist *next = (alloclist*)((uint8_t*)head + size);
                // there is space remaining
                _alsetfree(next);
                _alsetsize(next, rem);
                _alsetsize(head, size);
            } else {
                // take the entire chunk
                _alsetsize(head, chunk_size);
            }
            head++;
            return (uint8_t*)head;
        } else {
            // if get here, cur is not free, move on to the next
            head = (alloclist*)((uint8_t*)head + _algetsize(head));
        }
    }

    // if get here, didn't find any free chunks
    return NULL;
}

// allocate sub-page level allocation based on bytes and zero the memory
uint8_t *kzmalloc(uint64_t sz) {
    uint64_t o = (1 << 3) - 1;
    sz = (sz + o) & !o;
    uint8_t *ret = kmalloc(sz);
    if (ret != NULL) {
        for (int i = 0; i < sz; i++) {
            ret[i] = 0;
        }
    }
    return ret;
}

// free a sub-page level allocation
void kfree(uint8_t *ptr) {
    if (ptr != NULL) {
        alloclist *p = (alloclist*)ptr - 1;
        if (_alistaken(p)) {
            _alsetfree(p);
        }
        // after we free, see if we can combine adjacent free
        // spots to see if we can reduce fragmentation
        coalesce();
    }
}

// merge smaller chunks into a bigger chunk
void coalesce() {
    alloclist *head = KMEM_HEAD;
    alloclist *tail = (alloclist*)((uint8_t*)KMEM_HEAD + KMEM_ALLOC * PGSIZE);

    while (head < tail) {
        alloclist *next = (alloclist*)((uint8_t*)head + _algetsize(head));
        if (_algetsize(head) == 0) {
            // if this happen, then we have a bad heap (double free or something)
            // however, that will cause an infinite loop since the next pointer
            // will never move beyond the current location.
            break;
        } else if (next >= tail) {
            // we calculated the next by using the size given as _getsize(), however
            // this could push us past the tail. in that case, the size is wrong, 
            // hence break and stop doing what we need to do.
            break;
        } else if (_alisfree(head) && _alisfree(next)) {
            // this mean we have adjacent blocks needing to be freed. so we combine 
            // into one allocation
            _alsetsize(head, (_algetsize(head), _algetsize(next)));
        }
        // if get here, recalculate new head
        head = (alloclist*)((uint8_t*)head + _algetsize(head));
    }
}

// print kmem table
void printkmemtable() {
    alloclist *head = KMEM_HEAD;
    alloclist *tail = (alloclist*)((uint8_t*)KMEM_HEAD + KMEM_ALLOC * PGSIZE);
    while (head < tail) {
        printf("0x%x: len %d, taken %d\n", head, _algetsize(head), _alistaken(head));
        head = (alloclist*)((uint8_t*)head + _algetsize(head));
    }
}

// identify map range
// takes a contiguous allocation of memory and
// map it using PGSIZE, assumes that start <= end.
void maprange(pagetable_t pagetable, uint64_t start, uint64_t end, uint64_t bits) {
    uint64_t pa = PGROUNDDOWN(start);
    uint64_t np = (PGROUNDUP(end) - pa) / PGSIZE;
    for (int i = 0; i < np; i++) {
        pagemap(pagetable, pa, pa, bits, 0);
        pa += PGSIZE;
    }
}
