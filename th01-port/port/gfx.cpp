#include "gfx.h"
#include "pc98.h"
#include "planar.h"

static uint8_t g_color = 0;

uint8_t port_grcg_color(void) { return g_color; }
void port_grcg_set_color(uint8_t col) { g_color = col & 0x0F; port_grcg_set_tile_color(g_color); }

void port_fill_rect(int x1, int y1, int x2, int y2, uint8_t col)
{
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= RES_X) x2 = RES_X - 1;
    if (y2 >= RES_Y) y2 = RES_Y - 1;
    if (x1 > x2 || y1 > y2) return;

    for (int y = y1; y <= y2; y++) {
        int off = vram_offset_shift(x1, y);
        for (int x = x1; x <= x2; x++) {
            uint8_t bit = (uint8_t)(0x80 >> (x & 7));
            if (col & 0x1) VRAM_PLANE_B[off] |= bit; else VRAM_PLANE_B[off] &= ~bit;
            if (col & 0x2) VRAM_PLANE_R[off] |= bit; else VRAM_PLANE_R[off] &= ~bit;
            if (col & 0x4) VRAM_PLANE_G[off] |= bit; else VRAM_PLANE_G[off] &= ~bit;
            if (col & 0x8) VRAM_PLANE_E[off] |= bit; else VRAM_PLANE_E[off] &= ~bit;
            if ((x & 7) == 7) off++;
        }
    }
}
