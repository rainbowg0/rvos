#include "include/defs.h"

void main() {
    uartinit();
    uartputs("hello, os world!\n");

    while (1) {}
}