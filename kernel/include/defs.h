#ifndef RVOS_DEFS_H
#define RVOS_DEFS_H

#include "types.h"

struct page;

// uart.c
void uartinit();
int uartputc(char ch);
void uartputs(char *s);
int uartgetc();

// printf.c
int printf(const char *s, ...);
void panic();
void assert(bool flag);

// page.c
void pageinit();
void *pagealloc(int np);
void *pagezalloc(int np);
void pagedealloc(struct page *p);
void printpages();

#endif //RVOS_DEFS_H