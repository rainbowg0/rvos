#include "include/types.h"
#include "include/riscv.h"
#include "include/memlayout.h"

// UART control resigters are memory-mapped at
// address UART0. This macro returns the address
// of one of registers.
// volatile tells the compiler not to optimize anything that has
// to do with the volatile variable, three reasons to use it:
// 1. when interface with hardware that changes the value itself.
// 2. when there's another thread running that also uses the variable.
// 3. when there's a signal handler that might change the value of the variable.
#define REG(reg) ((volatile unsigned char*)(UART0 + reg))
#define ReadReg(reg) (*(REG(reg)))
#define WriteReg(reg, v) (*(REG(reg)) = (v))

// the UART control registers.
// some have different meanings for read/write.
#define RHR 0 // receive holding register (for input bytes)
#define THR 0 // transmit holding register (for output bytes)
#define DLL 0 // LSB of divisor latch (write mode)
#define IER 1 // interrupt enable register
#define DLM 1 // MSB of divisor latch (write mode)
#define FCR 2 // fifo control register
#define ISR 2 // interrupt status register
#define LCR 3 // line control register
#define MCR 4 // modem control register
#define LSR 5 // line status register
#define MSR 6 // modem status register
#define SPR 7 // scratch pad register

#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

/*
 * POWER UP DEFAULTS
 * IER = 0: TX/RX holding register interrupts are both disabled
 * ISR = 1: no interrupt penting
 * LCR = 0
 * MCR = 0
 * LSR = 60 HEX
 * MSR = BITS 0-3 = 0, BITS 4-7 = inputs
 * FCR = 0
 * TX = High
 * OP1 = High
 * OP2 = High
 * RTS = High
 * DTR = High
 * RXRDY = High
 * TXRDY = Low
 * INT = Low
 */

void uartinit() {
    // disable interrupts
    WriteReg(IER, 0x00);    

    // set the signal rate (BAUD)
    // divisor register DLL (divisor latch least) and
    // DLM (divisor latch most) have the same base address
    // as the receiver/transmitter and the interrupt enable
    // register. To change what the base address points to,
    // open the 'divisor latch' by writing 1 into the Divisor
    // Latch Access Bit (DLAB), which is bit index 7 of the
    // Line Control Register (LCR).
    WriteReg(LCR, LCR_BAUD_LATCH);
    WriteReg(DLL, 0x03);
    WriteReg(DLM, 0x00);

    // set the word length to 8 bits.
    // there are two bits in the LCR (line control register)
    // that control word length, set bit0 and bit1 as 1. 
    WriteReg(LCR, LCR_EIGHT_BITS);

    // enable fifo.
    // when data is added to the fifo, it retains the same
    // order so that when read from same fifo, get the first
    // piece of data entered.
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // enable transmit and receiver interrupts.
    // when data is added to the receiver, the CPU is notified
    // through an interrupt.
    WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);
}

int uartputc(char ch) {
    while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
        ;
    return WriteReg(THR, ch);
}

void uartputs(char *s) {
    while (*s) {
        uartputc(*s++);
    }
}

int uartgetc() {
    if (ReadReg(LSR) & 0x01) {
        return ReadReg(RHR);
    }

    return -1;
}
