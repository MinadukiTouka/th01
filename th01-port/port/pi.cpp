// Pi (.GRP) decoder for the TH01 port.
//
// Implements the documented Pi algorithm: a 16x16 move-to-front delta table
// followed by run-length repeats.  Output is 8-bit palette indices, which are
// expanded into the four PC-98 bitplanes.

#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "gfx.h"

#include <string.h>

namespace {

constexpr int kMaxData = 512 * 1024;
uint8_t g_data[kMaxData];
uint8_t g_pixels[640 * 400];

struct BitReader {
    const uint8_t *p;
    size_t bits;
    size_t pos;

    int get()
    {
        if (pos >= bits) return 0;
        int b = (p[pos >> 3] >> (7 - (pos & 7))) & 1;
        pos++;
        return b;
    }

    int getn(int n)
    {
        int v = 0;
        for (int i = 0; i < n; i++) v = (v << 1) | get();
        return v;
    }
};

int delta_code(BitReader &br)
{
    if (br.get()) return br.get();
    if (br.get() == 0) return 2 + br.get();
    if (br.get() == 0) return 4 + br.getn(2);
    return 8 + br.getn(3);
}

int repeat_loc(BitReader &br)
{
    if (br.get() == 0) {
        return br.get() ? 1 : 0;
    }
    if (br.get() == 0) return 2;
    return 3 + br.get();
}

int repeat_len(BitReader &br)
{
    int base = 1;
    int bits = 0;
    while (br.get() == 1) {
        base <<= 1;
        bits++;
    }
    return base + br.getn(bits);
}

uint8_t delta_decode(BitReader &br, uint8_t table[16][16], int &prev)
{
    int b = delta_code(br);
    if (b < 0) b = 0;
    if (b > 15) b = 15;
    uint8_t v = table[prev][b];
    for (int i = b; i > 0; i--) table[prev][i] = table[prev][i - 1];
    table[prev][0] = v;
    prev = v;
    return v;
}

void repeat_copy(
    uint8_t *out, int &outpos, int total, int width, int loc, int bytes
)
{
    if (bytes <= 0) return;
    int src = 0;
    switch (loc) {
    case 0:
        src = (outpos >= 4 && out[outpos - 1] != out[outpos - 2])
            ? (outpos - 4) : (outpos >= 2 ? outpos - 2 : 0);
        break;
    case 1: src = outpos - width; break;
    case 2: src = outpos - 2 * width; break;
    case 3: src = outpos - width + 1; break;
    case 4: src = outpos - width - 1; break;
    default: src = outpos - 2; break;
    }

    for (int i = 0; i < bytes && outpos < total; i++) {
        uint8_t v;
        if (src < 0) {
            v = out[(i & 1) % (outpos > 0 ? outpos : 1)];
        } else {
            v = out[src];
        }
        out[outpos++] = v;
        src++;
    }
}

void repeat_seq(
    BitReader &br, uint8_t *out, int &outpos, int total, int width,
    bool first
)
{
    int last_loc = -1;
    bool first_repeat = first;

    while (outpos < total) {
        int loc = repeat_loc(br);
        if (loc == last_loc) break;
        int len = repeat_len(br);
        int bytes = len * 2;
        if (first_repeat) {
            bytes -= 2;
            first_repeat = false;
        }
        repeat_copy(out, outpos, total, width, loc, bytes);
        last_loc = loc;
    }
}

} // namespace

extern "C" int port_pi_blit(
    void *buff, uint16_t size, int x, int y, int option,
    int (*read_func)(void *, unsigned)
)
{
    if (!read_func) return -1;

    int n = 0;
    while (n < kMaxData) {
        int got = read_func(g_data + n, (unsigned)(kMaxData - n));
        if (got <= 0) break;
        n += got;
    }
    if (n < 0x42 || g_data[0] != 'Z' || g_data[1] != 'N') return -1;

    int hdr = 2;
    while (hdr < n && g_data[hdr] != 0x1A) hdr++;
    hdr++;
    if (hdr + 18 > n) return -1;

    int width = (g_data[hdr + 11] << 8) | g_data[hdr + 12];
    int height = (g_data[hdr + 13] << 8) | g_data[hdr + 14];
    int bitdepth = g_data[hdr + 4];
    if (width <= 0 || height <= 0 || width > 640 || height > 400) return -1;
    if (bitdepth != 4) return -1;

    int pal_off = hdr + 15;
    if (pal_off + 48 > n) return -1;

    // grp.cpp reads the palette from (buff + 0x100).
    if (buff && size >= 0x130) {
        memcpy((uint8_t *)buff + 0x100, g_data + pal_off, 48);
    }

    int total = width * height;
    if (total > (int)sizeof(g_pixels)) return -1;

    uint8_t table[16][16];
    for (int a = 0; a < 16; a++)
        for (int b = 0; b < 16; b++)
            table[a][b] = (uint8_t)((16 + a - b) % 16);

    BitReader br;
    br.p = g_data + pal_off + 48;
    br.bits = (size_t)(n - (pal_off + 48)) * 8;
    br.pos = 0;

    int prev = 0;
    int outpos = 0;
    g_pixels[outpos++] = delta_decode(br, table, prev);
    g_pixels[outpos++] = delta_decode(br, table, prev);
    repeat_seq(br, g_pixels, outpos, total, width, true);
    if (outpos > 0) prev = g_pixels[outpos - 1];

    while (outpos < total) {
        g_pixels[outpos++] = delta_decode(br, table, prev);
        if (outpos >= total) break;
        g_pixels[outpos++] = delta_decode(br, table, prev);
        if (br.get()) continue;
        repeat_seq(br, g_pixels, outpos, total, width, false);
        if (outpos > 0) prev = g_pixels[outpos - 1];
    }

    int key = -1;
    if (option & 0x40) key = (option >> 8) & 0xFF;

    // Expand to the four bitplanes of the current page.
    for (int py = 0; py < height; py++) {
        int vy = y + py;
        if (vy < 0 || vy >= RES_Y) continue;
        for (int px = 0; px < width; px++) {
            int vx = x + px;
            if (vx < 0 || vx >= RES_X) continue;
            uint8_t idx = g_pixels[py * width + px] & 0x0F;
            if (key >= 0 && idx == key) continue;
            int off = (vy * ROW_SIZE) + (vx >> 3);
            uint8_t bit = (uint8_t)(0x80 >> (vx & 7));
            if (idx & 0x1) VRAM_PLANE_B[off] |= bit; else VRAM_PLANE_B[off] &= ~bit;
            if (idx & 0x2) VRAM_PLANE_R[off] |= bit; else VRAM_PLANE_R[off] &= ~bit;
            if (idx & 0x4) VRAM_PLANE_G[off] |= bit; else VRAM_PLANE_G[off] &= ~bit;
            if (idx & 0x8) VRAM_PLANE_E[off] |= bit; else VRAM_PLANE_E[off] &= ~bit;
        }
    }
    return 0;
}
