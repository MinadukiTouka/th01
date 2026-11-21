#pragma once
// Shift-JIS string helpers (Borland <mbstring.h> replacement).

#ifdef __cplusplus
extern "C" {
#endif

// Converts a Shift-JIS 2-byte code to the PC-98 JIS code (row << 8 | cell).
unsigned int _mbcjmstojis(unsigned int c);

// Returns the length of the multibyte character at [p] (1 or 2).
int _mbslen(const unsigned char *p);
const unsigned char *_mbsinc(const unsigned char *p);

#ifdef __cplusplus
}
#endif
