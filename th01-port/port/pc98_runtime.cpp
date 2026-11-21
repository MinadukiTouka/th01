// Minimal PC-98 real-mode / video runtime for the TH01 port.
//
// Provides:
//   - a 1 MiB flat dos_mem for MK_FP/peek/poke and heap allocations
//   - four VRAM bitplanes x two pages (640x400, 80 bytes/row)
//   - hardware page switching (ports A4h/A6h)
//   - a few I/O port hooks used by the game
//
// The actual drawing/blitting lives in port/master_gfx.cpp.

#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "video.h"

#include <string.h>

extern "C" {
uint8_t *dos_mem = nullptr;
}

// ---------------------------------------------------------------- globals
uint16_t _AX, _BX, _CX, _DX, _SI, _DI, _ES, _DS;
uint8_t _AL, _AH, _BL, _BH, _CL, _CH, _DL, _DH;
uint16_t _psp = 0;

namespace {

constexpr int kPlaneW = RES_X / BYTE_DOTS;   // 80
constexpr int kPlaneH = RES_Y;               // 400
constexpr int kPlaneSize = kPlaneW * kPlaneH; // 32000

uint8_t g_planes[PAGE_COUNT][PLANE_COUNT][kPlaneSize];
int g_access_page = 0;
int g_show_page = 0;
uint8_t g_grcg_mode = 0;
uint8_t g_grcg_tile[4] = {0, 0, 0, 0};
int g_grcg_tile_idx = 0;

uint8_t *plane_base(vram_plane_t p)
{
    return g_planes[g_access_page][(int)p];
}

} // namespace

// ---------------------------------------------------------------- planar.h
dots8_t *VRAM_PLANE[PLANE_COUNT] = {};
dots8_t *VRAM_PLANE_B = nullptr;
dots8_t *VRAM_PLANE_R = nullptr;
dots8_t *VRAM_PLANE_G = nullptr;
dots8_t *VRAM_PLANE_E = nullptr;

void port_video_refresh_plane_ptrs(void)
{
    VRAM_PLANE[PL_B] = g_planes[g_access_page][PL_B];
    VRAM_PLANE[PL_R] = g_planes[g_access_page][PL_R];
    VRAM_PLANE[PL_G] = g_planes[g_access_page][PL_G];
    VRAM_PLANE[PL_E] = g_planes[g_access_page][PL_E];
    VRAM_PLANE_B = VRAM_PLANE[PL_B];
    VRAM_PLANE_R = VRAM_PLANE[PL_R];
    VRAM_PLANE_G = VRAM_PLANE[PL_G];
    VRAM_PLANE_E = VRAM_PLANE[PL_E];
}


// ---------------------------------------------------------------- EGC copy
// The original EGC reads/writes all four bitplanes in parallel.  The C code
// only stores the B-plane word, so keep the R/G/E words from the last read
// and apply them on the matching write.
static bool g_egc_active = false;
static uint16_t g_egc_r = 0;
static uint16_t g_egc_g = 0;
static uint16_t g_egc_e = 0;


// ---------------------------------------------------------------- GRCG
// GRCG RMW writes replace the bits selected by the source mask in all four
// planes with the tile register.  The PC-98 code expresses fills/erases via
// grcg_put(), so this is the hot path for unputting sprites and cards.
void port_grcg_set_tile_color(uint8_t col)
{
    g_grcg_tile[0] = (col & 0x1) ? 0xFF : 0x00;
    g_grcg_tile[1] = (col & 0x2) ? 0xFF : 0x00;
    g_grcg_tile[2] = (col & 0x4) ? 0xFF : 0x00;
    g_grcg_tile[3] = (col & 0x8) ? 0xFF : 0x00;
}

void port_grcg_write(int offset, uint32_t src, int bit_count)
{
    int bytes = bit_count / 8;
    if (offset < 0 || offset + bytes > kPlaneSize) return;
    if ((g_grcg_mode & 0xC0) == 0) {
        for (int i = 0; i < bytes; i++) {
            VRAM_PLANE_B[offset + i] = (uint8_t)(src >> (8 * i));
        }
        return;
    }
    for (int i = 0; i < bytes; i++) {
        uint8_t mask = (uint8_t)(src >> (8 * i));
        for (int p = 0; p < PLANE_COUNT; p++) {
            uint8_t *dst = VRAM_PLANE[p] + offset + i;
            *dst = (uint8_t)((*dst & (uint8_t)~mask) | (g_grcg_tile[p] & mask));
        }
    }
}

void port_egc_set_active(bool active)
{
    g_egc_active = active;
}

uint16_t port_egc_read16(int offset)
{
    if (offset < 0 || offset + 2 > kPlaneSize) return 0;
    uint16_t v;
    memcpy(&v, VRAM_PLANE_B + offset, 2);
    if (g_egc_active) {
        memcpy(&g_egc_r, VRAM_PLANE_R + offset, 2);
        memcpy(&g_egc_g, VRAM_PLANE_G + offset, 2);
        memcpy(&g_egc_e, VRAM_PLANE_E + offset, 2);
    }
    return v;
}

void port_egc_write16(int offset, uint16_t value)
{
    if (offset < 0 || offset + 2 > kPlaneSize) return;
    memcpy(VRAM_PLANE_B + offset, &value, 2);
    if (g_egc_active) {
        memcpy(VRAM_PLANE_R + offset, &g_egc_r, 2);
        memcpy(VRAM_PLANE_G + offset, &g_egc_g, 2);
        memcpy(VRAM_PLANE_E + offset, &g_egc_e, 2);
    }
}

void port_video_set_access_page(int page)
{
    g_access_page = (page == 1) ? 1 : 0;
    port_video_refresh_plane_ptrs();
}

int port_video_access_page(void)
{
    return g_access_page;
}

void port_video_set_show_page(int page)
{
    g_show_page = (page != 0);
}

int port_video_show_page(void)
{
    return g_show_page;
}

uint8_t *port_video_plane(int page, int plane)
{
    if (page < 0 || page >= PAGE_COUNT || plane < 0 || plane >= PLANE_COUNT) {
        return nullptr;
    }
    return g_planes[page][plane];
}

bool port_video_is_plane_segment(uint16_t seg, int *plane_out)
{
    int p = -1;
    if (seg == SEG_PLANE_B) p = PL_B;
    else if (seg == SEG_PLANE_R) p = PL_R;
    else if (seg == SEG_PLANE_G) p = PL_G;
    else if (seg == SEG_PLANE_E) p = PL_E;
    if (p < 0) return false;
    if (plane_out) *plane_out = p;
    return true;
}

// ---------------------------------------------------------------- MK_FP
extern "C" void *port_mk_fp(uint16_t seg, uint16_t off)
{
    int plane = 0;
    if (port_video_is_plane_segment(seg, &plane)) {
        return plane_base((vram_plane_t)plane) + off;
    }
    if (!dos_mem) {
        dos_mem = (uint8_t *)calloc(1, 0x100000);
    }
    return dos_mem + ((uint32_t)seg << 4) + off;
}

extern "C" uint16_t port_fp_seg(const void *p)
{
    if (!p) return 0;
    // VRAM pointers: map back to the current page's plane segment.  The
    // offset is only guaranteed to be paragraph-aligned for the uses in the
    // original code, but returning a paragraph + byte offset works for
    // MK_FP(FP_SEG(p), FP_OFF(p)).
    for (int i = 0; i < PLANE_COUNT; i++) {
        const uint8_t *base = g_planes[g_access_page][i];
        const uint8_t *q = (const uint8_t *)p;
        if (q >= base && q < base + kPlaneSize) {
            static const uint16_t segs[PLANE_COUNT] = {
                SEG_PLANE_B, SEG_PLANE_R, SEG_PLANE_G, SEG_PLANE_E
            };
            return (uint16_t)(segs[i] + ((q - base) >> 4));
        }
    }
    if (dos_mem) {
        const uint8_t *q = (const uint8_t *)p;
        if (q >= dos_mem && q < dos_mem + 0x100000) {
            uint32_t v = (uint32_t)(q - dos_mem);
            return (uint16_t)(v >> 4);
        }
    }
    // Stack/global pointers: the legacy code only passes these to no-op
    // BIOS calls, so a zero segment is fine.
    return 0;
}

extern "C" uint16_t port_fp_off(const void *p)
{
    if (!p) return 0;
    for (int i = 0; i < PLANE_COUNT; i++) {
        const uint8_t *base = g_planes[g_access_page][i];
        const uint8_t *q = (const uint8_t *)p;
        if (q >= base && q < base + kPlaneSize) {
            return (uint16_t)((q - base) & 0xF);
        }
    }
    if (dos_mem) {
        const uint8_t *q = (const uint8_t *)p;
        if (q >= dos_mem && q < dos_mem + 0x100000) {
            return (uint16_t)((q - dos_mem) & 0xF);
        }
    }
    return (uint16_t)(uintptr_t)p;
}

extern "C" int16_t port_peek(uint16_t seg, uint16_t off)
{
    return *(int16_t *)port_mk_fp(seg, off);
}
extern "C" int8_t port_peekb(uint16_t seg, uint16_t off)
{
    return *(int8_t *)port_mk_fp(seg, off);
}
extern "C" void port_poke(uint16_t seg, uint16_t off, int16_t v)
{
    *(int16_t *)port_mk_fp(seg, off) = v;
}
extern "C" void port_pokeb(uint16_t seg, uint16_t off, int8_t v)
{
    *(int8_t *)port_mk_fp(seg, off) = v;
}

// ---------------------------------------------------------------- I/O ports
extern "C" uint8_t port_inportb(uint16_t port)
{
    // KCG glyph read is handled by the font layer; return 0 here.
    (void)port;
    return 0;
}
extern "C" uint16_t port_inportw(uint16_t port)
{
    (void)port;
    return 0;
}
extern "C" void port_outportb(uint16_t port, uint8_t v)
{
    switch (port) {
    case 0xA6: // graphics access page
        port_video_set_access_page(v & 1);
        break;
    case 0xA4: // graphics show page
        port_video_set_show_page(v & 1);
        break;
    case 0x7C: // GRCG mode
        g_grcg_mode = v;
        g_grcg_tile_idx = 0;
        break;
    case 0x7E: // GRCG tile register (B, R, G, E)
        g_grcg_tile[g_grcg_tile_idx & 3] = v;
        g_grcg_tile_idx++;
        break;
    default:
        break;
    }
}
extern "C" void port_outportw(uint16_t port, uint16_t v)
{
    (void)port;
    (void)v;
}

// ---------------------------------------------------------------- BIOS stubs
extern "C" int port_key_group_sense(int group);

extern "C" int port_int86(int no, union REGS *in, union REGS *out)
{
    if (out) *out = *in;
    if (no == 0x18 && in && in->h.ah == 0x04 && out) {
        out->h.ah = (uint8_t)port_key_group_sense(in->h.al);
        out->h.al = 0;
    }
    return 0;
}
extern "C" int_handler_t port_getvect(int no)
{
    (void)no;
    return nullptr;
}
extern "C" void port_setvect(int no, int_handler_t h)
{
    (void)no;
    (void)h;
}
extern "C" void port_segread(struct SREGS *s)
{
    if (s) { s->es = s->cs = s->ss = s->ds = 0; }
}

// Initialize at static-init time so that VRAM_PLANE is valid before main().
struct PortVideoInit {
    PortVideoInit()
    {
        if (!dos_mem) dos_mem = (uint8_t *)calloc(1, 0x100000);
        port_video_refresh_plane_ptrs();
    }
};
static PortVideoInit g_port_video_init;
