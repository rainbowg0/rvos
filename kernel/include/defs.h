#ifndef RVOS_DEFS_H
#define RVOS_DEFS_H

// uart.c
void uartinit();
int uartputc(char ch);
void uartputs(char *s);
int uartgetc();

#endif //RVOS_DEFS_H