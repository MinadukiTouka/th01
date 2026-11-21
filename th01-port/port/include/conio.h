#pragma once
// Portable replacement for Borland <conio.h>.
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int getch(void) { return 0; }
static inline int getche(void) { return 0; }
static inline int kbhit(void) { return 0; }
static inline int putch(int c) { return c; }
static inline int cputs(const char *s) { (void)s; return 0; }

#ifdef __cplusplus
}
#endif
