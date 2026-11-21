// Shift-JIS helpers used by the TH01 text layer.

#include "mbctype.h"
#include "mbstring.h"

extern "C" {

int _ismbblead(unsigned int c)
{
    c &= 0xFF;
    return ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC));
}

int _ismbbtrail(unsigned int c)
{
    c &= 0xFF;
    return ((c >= 0x40 && c <= 0x7E) || (c >= 0x80 && c <= 0xFC));
}

int _ismbbkana(unsigned int c)
{
    c &= 0xFF;
    return (c >= 0xA1 && c <= 0xDF);
}

int _ismbcalnum(unsigned int c) { return (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A); }
int _ismbcdigit(unsigned int c) { return (c >= 0x30 && c <= 0x39); }
int _ismbcspace(unsigned int c) { return (c == 0x20); }

unsigned int _mbcjmstojis(unsigned int c)
{
    unsigned int hi = (c >> 8) & 0xFF;
    unsigned int lo = c & 0xFF;
    unsigned int row;
    unsigned int cell;

    if (hi >= 0x81 && hi <= 0x9F) {
        row = (hi - 0x81) * 2 + 0x21;
    } else if (hi >= 0xE0 && hi <= 0xFC) {
        row = (hi - 0xC1) * 2 + 0x21;
    } else {
        return 0;
    }

    if (lo >= 0x9F) {
        row += 1;
        cell = lo - 0x7E;
    } else if (lo >= 0x80) {
        cell = lo - 0x20;
    } else {
        cell = lo - 0x1F;
    }
    return (row << 8) | (cell & 0xFF);
}

int _mbslen(const unsigned char *p)
{
    if (!p || !*p) return 0;
    return _ismbblead(*p) ? 2 : 1;
}

const unsigned char *_mbsinc(const unsigned char *p)
{
    return p + _mbslen(p);
}

} // extern "C"
