#include "include/defs.h"

void kmain() {
    uartputs("hello, os world!\n");

    while (1) {}
}

// kinit() runs in super-duper mode
// kinit() is to get us into supervisor mode
// as soon as possible.
void kinit() {
    uartinit();
    pageinit();
    printpages();
}