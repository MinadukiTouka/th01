#pragma once
// Shift-JIS classification helpers (Borland <mbctype.h> replacement).

#ifdef __cplusplus
extern "C" {
#endif

int _ismbblead(unsigned int c);
int _ismbbtrail(unsigned int c);
int _ismbbkana(unsigned int c);
int _ismbcalnum(unsigned int c);
int _ismbcdigit(unsigned int c);
int _ismbcspace(unsigned int c);

#ifdef __cplusplus
}
#endif
