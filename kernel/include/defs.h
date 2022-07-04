#ifndef RVOS_DEFS_H
#define RVOS_DEFS_H

#include "types.h"
#include "riscv.h"

struct page;
struct alloclist;

// uart.c
void uartinit();
int uartputc(char ch);
void uartputs(char *s);
int uartgetc();

// printf.c
int printf(const char *s, ...);
void panic(char *s);
void assert(bool flag);

// page.c
void pageinit();
void *pagealloc(int np);
void *pagezalloc(int np);
void pagedealloc(struct page *p);
void printpagealloc();
void pagemap(pagetable_t pagetable, uint64_t va, uint64_t pa, uint64_t bits, uint64_t level);
void pageumap(pagetable_t pagetable);
uint64_t va2pa(pagetable_t pagetable, uint64_t vaddr);
uint64_t getallocstart();

// pagetest.c
void pagetest();

// kmem.c
void kmeminit();
uint8_t *kmalloc(uint64_t sz);
uint8_t *kzmalloc(uint64_t sz);
void kfree(uint8_t *ptr);
void printkmemtable();
void coalesce();
void maprange(pagetable_t pagetable, uint64_t start, uint64_t end, uint64_t bits);
struct alloclist *gethead();
uint64_t getnumalloc();
pagetable_t gettable();

#endif //RVOS_DEFS_H