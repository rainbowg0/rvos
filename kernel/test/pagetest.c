#include "../include/defs.h"
#include "../include/memlayout.h"
#include "../include/types.h"

void largealloc() {
	printf("\nlargealloc test start...\n");
	// allocate maximum number of pages
	int num = PGROUNDDOWN(HEAP_START + HEAP_SIZE - getallocstart()) / PGSIZE;
	printf("try to alloc %d pages\n", num);
	void *ptr1 = pagealloc(num-1);
	if (ptr1 == NULL) {
		panic("largealloc: can't alloc maxsize");
	}

	// allocate more is not allowed
	void *ptr2 = pagealloc(1);
	if (ptr2 != NULL) {
		panic("largealloc: alloc out of size should return NULL");
	}

	pagedealloc(ptr1);
	printf("largetalloc test: pass!\n\n");
}

void halfalloc() {
	printf("\nhalfalloc test start....\n");
	int num = PGROUNDDOWN(HEAP_START + HEAP_SIZE - getallocstart()) / PGSIZE;
	printf("try to alloc %d pages\n", num / 2 + 10);
	void *ptr1 = pagealloc(num / 2 + 10);
	if (ptr1 == NULL) {
		panic("halfalloc: can't alloc half size");
	}

	void *ptr2 = pagealloc(num / 2);
	if (ptr2 != NULL) {
		panic("largealloc: alloc out of size should return NULL");
	}

	pagedealloc(ptr1);
	printf("halfalloc test: pass!\n\n");
}

void alloczero() {
	printf("\nlloczero test start...\n");

	uint64_t *ptr = (uint64_t*)pagezalloc(1);
	for (int i = 0; i < PGSIZE / 8; i++) {
		if (ptr[i] != 0) {
			panic("alloczero: not zero");
		}
	}

	pagedealloc((void*)ptr);
	printf("alloczero test pass!\n\n");
}

void havedealloc() {
	printf("\nhavedealloc test start...\n");

	void *ptr1 = pagealloc(1);
	uint64_t addr1 = (uint64_t) ptr1;

	pagedealloc(ptr1);

	void *ptr2 = pagealloc(1);
	uint64_t addr2 = (uint64_t) ptr2;
	if (addr1 != addr2) {
		panic("havedealloc: not the same");
	}

	pagedealloc(ptr2);
	printf("havedealloc test pass!\n\n");
}

void largescale() {
	printf("\nlargescale test: start...\n");

	int num = PGROUNDDOWN(HEAP_START + HEAP_SIZE - getallocstart()) / PGSIZE;
	void *ptr = NULL;
	for (int i = 1; i <= 1000; i++) {
		if (i % 2 == 1) {
			ptr = pagealloc(num-1);
			if (ptr == NULL) {
				printf("%d ", i);
				panic("largescale: can't alloc maxsize");
			}
		} else {
			pagedealloc(ptr);
		}
	}

	printf("largescale test: pass!\n\n");
}

void pagetest() {
	largealloc();
	halfalloc();
	alloczero();
	havedealloc();
	largescale();
}

