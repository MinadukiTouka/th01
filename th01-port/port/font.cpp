// PC-98 glyph provider.
//
// Android: rasterizes glyphs from the system CJK font with FreeType.
// Desktop: blank fallback (the desktop harness is for logic/VRAM validation).

#include "platform/x86real/pc98/font.hpp"
#include <string.h>
#include <stdlib.h>

extern const uint32_t th01_jis2uni[94 * 94];

#if defined(__ANDROID__)
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library g_lib;
static FT_Face g_face;
static bool g_tried;

static void ensure_face()
{
    if (g_tried) return;
    g_tried = true;
    if (FT_Init_FreeType(&g_lib) != 0) return;

    const char *candidates[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/NotoSansCJK.ttc",
        "/system/fonts/NotoSansCJKjp-Regular.otf",
        "/system/fonts/NotoSansJP-Regular.otf",
        "/system/fonts/DroidSansFallback.ttf",
        "/system/fonts/DroidSansJapanese.ttf",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        if (FT_New_Face(g_lib, candidates[i], 0, &g_face) == 0) {
            FT_Set_Pixel_Sizes(g_face, 0, 16);
            return;
        }
    }
    g_face = nullptr;
}

static void render_unicode(
    uint32_t cp, int max_w, int max_h, uint16_t rows[16]
)
{
    memset(rows, 0, sizeof(uint16_t) * 16);
    ensure_face();
    if (!g_face) return;
    if (FT_Load_Char(g_face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) != 0) {
        return;
    }
    FT_Bitmap &bm = g_face->glyph->bitmap;
    int w = bm.width < max_w ? bm.width : max_w;
    int h = bm.rows < max_h ? bm.rows : max_h;
    int pitch = bm.pitch;
    for (int y = 0; y < h; y++) {
        const unsigned char *src = bm.buffer + y * pitch;
        for (int x = 0; x < w; x++) {
            if (src[x >> 3] & (0x80 >> (x & 7))) {
                rows[y] |= (uint16_t)(0x8000 >> x);
            }
        }
    }
}

static uint32_t jis_to_unicode(uint16_t jis)
{
    int row = (jis >> 8) & 0xFF;
    int cell = jis & 0xFF;
    if (row < 0x21 || row > 0x7E || cell < 0x21 || cell > 0x7E) return 0;
    return th01_jis2uni[(row - 0x21) * 94 + (cell - 0x21)];
}
#endif

void font_read(font_glyph_t &glyph, jis_t jis)
{
    memset(&glyph, 0, sizeof(glyph));
    glyph.tag.h = GLYPH_H;
    glyph.tag.w = GLYPH_FULL_W;
#if defined(__ANDROID__)
    // JIS row 0x29 is ZUN's halfwidth ANK block.
    if ((uint16_t)jis >= 0x2900 && (uint16_t)jis <= 0x29FF) {
        uint32_t cp = (uint16_t)jis - 0x2900;
        if (cp == 0) cp = ' ';
        uint16_t rows[16];
        render_unicode(cp, 8, 16, rows);
        glyph.tag.w = GLYPH_HALF_W;
        for (int i = 0; i < 16; i++) {
            glyph.ank_8x16.row[i] = (uint8_t)(rows[i] >> 8);
        }
        return;
    }
    uint32_t cp = jis_to_unicode((uint16_t)jis);
    if (!cp) cp = ' ';
    uint16_t rows[16];
    render_unicode(cp, 16, 16, rows);
    for (int i = 0; i < 16; i++) {
        glyph.kanji.row[i] = (uint16_t)((rows[i] >> 8) | (rows[i] << 8));
    }
#endif
}

void font_read(font_glyph_ank_8x8_t &glyph, ank_t ank)
{
    (void)ank;
    memset(&glyph, 0, sizeof(glyph));
    glyph.h = GLYPH_HALF_H;
    glyph.w = GLYPH_HALF_W;
}

void font_read(font_glyph_ank_8x16_t &glyph, ank_t ank)
{
    memset(&glyph, 0, sizeof(glyph));
    glyph.h = GLYPH_H;
    glyph.w = GLYPH_HALF_W;
#if defined(__ANDROID__)
    uint32_t cp = ank;
    if (ank >= 0xA1 && ank <= 0xDF) cp = 0xFF61 + (ank - 0xA1);
    uint16_t rows[16];
    render_unicode(cp, 8, 16, rows);
    for (int i = 0; i < 16; i++) glyph.dots.row[i] = (uint8_t)(rows[i] >> 8);
#endif
}
