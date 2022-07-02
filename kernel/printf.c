/*
 * ref: https://github.com/cccriscv/mini-riscv-os/blob/master/05-Preemptive/lib.c
 */

#include <stdarg.h>

#include "include/defs.h"
#include "include/types.h"

static char outbuf[1000];

static int _vsnprintf(char *out, size_t n, const char *s, va_list vl) {
    int format = 0;
    int longarg = 0;
    size_t pos = 0;
    for (; *s; s++) {
        if (format) {
            long num;
            switch (*s)
            {
            case 'l':
                longarg = 1;
                break;
            case 'p':
                longarg = 1;
                if (out && pos < n) {
                    out[pos] = '0';
                }
                pos++;
                if (out && pos < n) {
                    out[pos] = 'x';
                }
                pos++;
            case 'x':
                num = longarg ? va_arg(vl, long) : va_arg(vl, int);
                int hexdigits = 2 * (longarg ? sizeof(long) : sizeof(int)) - 1;
                for (int i = hexdigits; i >= 0; i--) {
                    int d = (num >> (4 * i)) & 0xf;
                    if (out && pos < n) {
                        out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
                    }
                    pos++;
                }
                longarg = 0;
                format = 0;
                break;
            case 'd':
                num = longarg ? va_arg(vl, long) : va_arg(vl, int);
                if (num < 0) {
                    num = -num;
                    if (out && pos < n) {
                        out[pos] = '-';
                    }
                    pos++;
                }
                long digits = 1;
                for (long nn = num; nn /= 10; digits++)
                    ;
                for (int i = digits - 1; i >= 0; i--) {
                    if (out && pos + i < n) {
                        out[pos + i] = '0' + (num % 10);
                    }
                    num /= 10;
                }
                pos += digits;
                longarg = 0;
                format = 0;
                break;
            case 's' :
            {
                const char *s2 = va_arg(vl, const char *);
                while (*s2) {
                    if (out && pos < n) {
                        out[pos] = *s2;
                    }
                    pos++;
                    s2++;
                }
                longarg = 0;
                format = 0;
                break;
            }
            case 'c':
                if (out && pos < n) {
                    out[pos] = (char)va_arg(vl, int);
                }
                pos++;
                longarg = 0;
                format = 0;
                break;
            default:
                break;
            }
        } else if (*s == '%') {
            format = 1;
        } else {
            if (out && pos < n) {
                out[pos] = *s;
            }
            pos++;
        }
    }
    if (out && pos < n) {
        out[pos] = 0;
    } else if (out && n) {
        out[n-1] = 0;
    }
    return pos;
}

int _vprintf(const char *s, va_list vl) {
    int res = _vsnprintf(NULL, -1, s, vl);
    if (res + 1 >= sizeof(outbuf)) {
        uartputs("error: output string size overflow!\n");
        while (1)
            ;
    }
    _vsnprintf(outbuf, res + 1, s, vl);
    uartputs(outbuf);
    return res;
}

int printf(const char *s, ...) {
    int res = 0;
    va_list vl;
    va_start(vl, s);
    res = _vprintf(s, vl);
    va_end(vl);
    return res;
}

void panic(char *s) {
    printf("panic: ");
    printf(s);
    printf("\n");
    while (1)
        ;
}

void assert(bool flag) {
    if (!flag) {
        panic("assert false");
    }
}