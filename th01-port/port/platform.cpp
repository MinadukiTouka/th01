// Platform-class shims used by TH01 but implemented by the TH02+ layer in
// ReC98: clipping, the optimized sprite blitter, EGCCopy and GRCG::set_color.

#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "platform/grp_clip.hpp"
#include "platform/x86real/pc98/blitter.hpp"
#include "platform/x86real/pc98/egc.hpp"
#include "platform/x86real/pc98/grcg.hpp"
#include "gfx.h"
#include "video.h"
#include "th01/pc98/entry.hpp"

#include <string.h>

LTRB<vram_x_t, screen_y_t> GrpClip = { 0, 0, (vram_x_t)ROW_SIZE, (screen_y_t)RES_Y };

void Grp_SetClip(const LTRB<screen_x_t, screen_y_t> *region)
{
    if (!region) {
        GrpClip.left = 0;
        GrpClip.top = 0;
        GrpClip.right = ROW_SIZE;
        GrpClip.bottom = RES_Y;
    } else {
        GrpClip.left = (region->left >> BYTE_BITS);
        GrpClip.top = region->top;
        GrpClip.right = (region->right >> BYTE_BITS);
        GrpClip.bottom = region->bottom;
    }
}

// ---------------------------------------------------------------- blitter
blit_source_t blit_source;
blit_state_t blit_state;

static int plane_index_for_seg(seg_t seg)
{
    switch (seg) {
    case SEG_PLANE_B: return PL_B;
    case SEG_PLANE_R: return PL_R;
    case SEG_PLANE_G: return PL_G;
    case SEG_PLANE_E: return PL_E;
    default: return -1;
    }
}

static void blit_write(seg_t plane_seg)
{
    int plane = plane_index_for_seg(plane_seg);
    if (plane < 0 || blit_state.w_clipped <= 0 || blit_state.h_clipped <= 0) {
        return;
    }
    const uint8_t *src = (const uint8_t *)blit_source.dots_start.fp;
    if (!src) return;
    uint8_t *dst = VRAM_PLANE[plane] + blit_state.vo;
    int w = blit_state.w_clipped;
    int h = blit_state.h_clipped;
    int soff = blit_state.sprite_offset;
    uint8_t col = port_grcg_color();

    for (int y = 0; y < h; y++) {
        const uint8_t *s = src + soff + y * blit_source.stride;
        uint8_t *d = dst + y * ROW_SIZE;
        for (int x = 0; x < w; x++) {
            uint8_t mask = s[x];
            if (!mask) continue;
            if (col & 0x1) d[x] |= mask; else d[x] &= ~mask;
            if (col & 0x2) VRAM_PLANE_R[blit_state.vo + y * ROW_SIZE + x] |= mask;
            else           VRAM_PLANE_R[blit_state.vo + y * ROW_SIZE + x] &= ~mask;
            if (col & 0x4) VRAM_PLANE_G[blit_state.vo + y * ROW_SIZE + x] |= mask;
            else           VRAM_PLANE_G[blit_state.vo + y * ROW_SIZE + x] &= ~mask;
            if (col & 0x8) VRAM_PLANE_E[blit_state.vo + y * ROW_SIZE + x] |= mask;
            else           VRAM_PLANE_E[blit_state.vo + y * ROW_SIZE + x] &= ~mask;
        }
    }
}

static Blitter g_blitter = { blit_write, blit_write };

const Blitter *blitter_init_clip(vram_x_t left, vram_y_t top)
{
    int w = blit_source.w;
    int h = blit_source.h;
    int sprite_offset = blit_source.offset;
    int dst_x = left;
    int dst_y = top;

    if (top < GrpClip.top) {
        int skip = GrpClip.top - top;
        if (skip >= h) return nullptr;
        sprite_offset += skip * blit_source.stride;
        h -= skip;
        dst_y = GrpClip.top;
    }
    if (dst_y + h > GrpClip.bottom) {
        h = GrpClip.bottom - dst_y;
        if (h <= 0) return nullptr;
    }
    if (left < GrpClip.left) {
        int skip = GrpClip.left - left;
        if (skip >= w) return nullptr;
        sprite_offset += skip;
        w -= skip;
        dst_x = GrpClip.left;
    }
    if (dst_x + w > GrpClip.right) {
        w = GrpClip.right - dst_x;
        if (w <= 0) return nullptr;
    }

    blit_state.vo = vram_offset_shift(dst_x * BYTE_DOTS, dst_y);
    blit_state.sprite_offset = sprite_offset;
    blit_state.w_clipped = w;
    blit_state.h_clipped = h;
    return &g_blitter;
}

const Blitter &blitter_init_noclip(vram_x_t left, vram_y_t top)
{
    blit_state.vo = vram_offset_shift(left * BYTE_DOTS, top);
    blit_state.sprite_offset = blit_source.offset;
    blit_state.w_clipped = blit_source.w;
    blit_state.h_clipped = blit_source.h;
    return g_blitter;
}

// ---------------------------------------------------------------- EGCCopy
EGCCopy::EGCCopy() {}
EGCCopy::~EGCCopy() {}

static void copy_rect_on_page(int dst_page, int src_page,
                              int sx, int sy, int dx, int dy, int w, int h)
{
    if (w <= 0 || h <= 0) return;

    // Clip to the 640x400 VRAM surface.  The PC-98 EGC ignored out-of-range
    // source/destination rows, but the software emulation must not walk past
    // the plane buffer (the final-boss pan uses top=400 to scroll in).
    if (sx < 0) { dx -= sx; w += sx; sx = 0; }
    if (dx < 0) { sx -= dx; w += dx; dx = 0; }
    if (sy < 0) { dy -= sy; h += sy; sy = 0; }
    if (dy < 0) { sy -= dy; h += dy; dy = 0; }
    if (sx >= RES_X || dx >= RES_X || sy >= RES_Y || dy >= RES_Y) return;
    if (w > RES_X - sx) w = RES_X - sx;
    if (w > RES_X - dx) w = RES_X - dx;
    if (h > RES_Y - sy) h = RES_Y - sy;
    if (h > RES_Y - dy) h = RES_Y - dy;
    if (w <= 0 || h <= 0) return;

    int bytes = (w + 7) / 8;
    for (int p = 0; p < PLANE_COUNT; p++) {
        uint8_t *src = port_video_plane(src_page, p);
        uint8_t *dst = port_video_plane(dst_page, p);
        for (int y = 0; y < h; y++) {
            int so = vram_offset_shift(sx, sy + y);
            int doff = vram_offset_shift(dx, dy + y);
            memmove(dst + doff, src + so, bytes);
        }
    }
}

void EGCCopy::rect(
    screen_x_t src_left, vram_y_t src_top,
    screen_x_t dst_left, vram_y_t dst_top,
    pixel_t w, pixel_t h
)
{
    int page = port_video_access_page();
    copy_rect_on_page(page, page, src_left, src_top, dst_left, dst_top, w, h);
}

void EGCCopy::rect_interpage(
    screen_x_t src_left, vram_y_t src_top,
    screen_x_t dst_left, vram_y_t dst_top,
    pixel_t w, pixel_t h, page_t src_page
)
{
    copy_rect_on_page(!src_page, src_page, src_left, src_top, dst_left, dst_top, w, h);
    port_video_set_access_page(!src_page);
}

void EGCCopy::rect_interpage(
    screen_x_t left, vram_y_t top, pixel_t w, pixel_t h, page_t src_page
)
{
    rect_interpage(left, top, left, top, w, h, src_page);
}

// ---------------------------------------------------------------- GRCG
void GRCG::set_color(vc_t col)
{
    port_grcg_set_color(col);
}

// ---------------------------------------------------------------- entries
static entrypoint_t g_pending_entrypoint = EP_OP;
static bool g_has_pending_entrypoint = false;

void entrypoint_exec(entrypoint_t entrypoint)
{
    g_pending_entrypoint = entrypoint;
    g_has_pending_entrypoint = true;
}

bool port_entrypoint_pending(entrypoint_t *out)
{
    if (!g_has_pending_entrypoint) return false;
    if (out) *out = g_pending_entrypoint;
    g_has_pending_entrypoint = false;
    return true;
}

bool port_entrypoint_has_pending(void)
{
    return g_has_pending_entrypoint;
}
