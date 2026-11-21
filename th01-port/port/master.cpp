// Minimal master.lib compatibility layer for TH01.
//
// This is deliberately small: it implements the symbols TH01 actually links
// against. Rendering/text/audio get fleshed out in dedicated files.

#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/piloadc/piloadm.hpp"
#include "platform/x86real/pc98/font.hpp"
#include "gfx.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------- file I/O
static FILE *g_file = nullptr;

extern "C" int file_ropen(const char *filename)
{
    g_file = fopen(filename, "rb");
    return g_file ? 1 : 0;
}

extern "C" int file_read(void *buf, unsigned wsize)
{
    if (!g_file) return 0;
    return (int)fread(buf, 1, wsize, g_file);
}

extern "C" void file_seek(long pos, int dir)
{
    if (!g_file) return;
    int whence = (dir == 0) ? SEEK_SET : (dir == 1) ? SEEK_CUR : SEEK_END;
    fseek(g_file, pos, whence);
}

extern "C" void file_close(void)
{
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
}

extern "C" int file_create(const char *filename)
{
    g_file = fopen(filename, "wb");
    fprintf(stderr, "file_create %s -> %p\n", filename, (void *)g_file);
    return g_file ? 1 : 0;
}

extern "C" int file_write(const void *buf, unsigned wsize)
{
    if (!g_file) { fprintf(stderr, "file_write no file\n"); return 0; }
    int n = (int)fwrite(buf, 1, wsize, g_file);
    fprintf(stderr, "file_write %u -> %d\n", wsize, n);
    return n;
}

extern "C" int file_exist(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

// ---------------------------------------------------------------- math
extern "C" {
int32_t random_seed = 0;
const short SinTable8[256] = {
#define S8(i) ((short)lround(sin((i) * 2.0 * 3.14159265358979323846 / 256.0) * 256.0))
    S8(0),S8(1),S8(2),S8(3),S8(4),S8(5),S8(6),S8(7),
    S8(8),S8(9),S8(10),S8(11),S8(12),S8(13),S8(14),S8(15),
    S8(16),S8(17),S8(18),S8(19),S8(20),S8(21),S8(22),S8(23),
    S8(24),S8(25),S8(26),S8(27),S8(28),S8(29),S8(30),S8(31),
    S8(32),S8(33),S8(34),S8(35),S8(36),S8(37),S8(38),S8(39),
    S8(40),S8(41),S8(42),S8(43),S8(44),S8(45),S8(46),S8(47),
    S8(48),S8(49),S8(50),S8(51),S8(52),S8(53),S8(54),S8(55),
    S8(56),S8(57),S8(58),S8(59),S8(60),S8(61),S8(62),S8(63),
    S8(64),S8(65),S8(66),S8(67),S8(68),S8(69),S8(70),S8(71),
    S8(72),S8(73),S8(74),S8(75),S8(76),S8(77),S8(78),S8(79),
    S8(80),S8(81),S8(82),S8(83),S8(84),S8(85),S8(86),S8(87),
    S8(88),S8(89),S8(90),S8(91),S8(92),S8(93),S8(94),S8(95),
    S8(96),S8(97),S8(98),S8(99),S8(100),S8(101),S8(102),S8(103),
    S8(104),S8(105),S8(106),S8(107),S8(108),S8(109),S8(110),S8(111),
    S8(112),S8(113),S8(114),S8(115),S8(116),S8(117),S8(118),S8(119),
    S8(120),S8(121),S8(122),S8(123),S8(124),S8(125),S8(126),S8(127),
    S8(128),S8(129),S8(130),S8(131),S8(132),S8(133),S8(134),S8(135),
    S8(136),S8(137),S8(138),S8(139),S8(140),S8(141),S8(142),S8(143),
    S8(144),S8(145),S8(146),S8(147),S8(148),S8(149),S8(150),S8(151),
    S8(152),S8(153),S8(154),S8(155),S8(156),S8(157),S8(158),S8(159),
    S8(160),S8(161),S8(162),S8(163),S8(164),S8(165),S8(166),S8(167),
    S8(168),S8(169),S8(170),S8(171),S8(172),S8(173),S8(174),S8(175),
    S8(176),S8(177),S8(178),S8(179),S8(180),S8(181),S8(182),S8(183),
    S8(184),S8(185),S8(186),S8(187),S8(188),S8(189),S8(190),S8(191),
    S8(192),S8(193),S8(194),S8(195),S8(196),S8(197),S8(198),S8(199),
    S8(200),S8(201),S8(202),S8(203),S8(204),S8(205),S8(206),S8(207),
    S8(208),S8(209),S8(210),S8(211),S8(212),S8(213),S8(214),S8(215),
    S8(216),S8(217),S8(218),S8(219),S8(220),S8(221),S8(222),S8(223),
    S8(224),S8(225),S8(226),S8(227),S8(228),S8(229),S8(230),S8(231),
    S8(232),S8(233),S8(234),S8(235),S8(236),S8(237),S8(238),S8(239),
    S8(240),S8(241),S8(242),S8(243),S8(244),S8(245),S8(246),S8(247),
    S8(248),S8(249),S8(250),S8(251),S8(252),S8(253),S8(254),S8(255)
#undef S8
};
const short CosTable8[256] = {
#define C8(i) ((short)lround(cos((i) * 2.0 * 3.14159265358979323846 / 256.0) * 256.0))
    C8(0),C8(1),C8(2),C8(3),C8(4),C8(5),C8(6),C8(7),
    C8(8),C8(9),C8(10),C8(11),C8(12),C8(13),C8(14),C8(15),
    C8(16),C8(17),C8(18),C8(19),C8(20),C8(21),C8(22),C8(23),
    C8(24),C8(25),C8(26),C8(27),C8(28),C8(29),C8(30),C8(31),
    C8(32),C8(33),C8(34),C8(35),C8(36),C8(37),C8(38),C8(39),
    C8(40),C8(41),C8(42),C8(43),C8(44),C8(45),C8(46),C8(47),
    C8(48),C8(49),C8(50),C8(51),C8(52),C8(53),C8(54),C8(55),
    C8(56),C8(57),C8(58),C8(59),C8(60),C8(61),C8(62),C8(63),
    C8(64),C8(65),C8(66),C8(67),C8(68),C8(69),C8(70),C8(71),
    C8(72),C8(73),C8(74),C8(75),C8(76),C8(77),C8(78),C8(79),
    C8(80),C8(81),C8(82),C8(83),C8(84),C8(85),C8(86),C8(87),
    C8(88),C8(89),C8(90),C8(91),C8(92),C8(93),C8(94),C8(95),
    C8(96),C8(97),C8(98),C8(99),C8(100),C8(101),C8(102),C8(103),
    C8(104),C8(105),C8(106),C8(107),C8(108),C8(109),C8(110),C8(111),
    C8(112),C8(113),C8(114),C8(115),C8(116),C8(117),C8(118),C8(119),
    C8(120),C8(121),C8(122),C8(123),C8(124),C8(125),C8(126),C8(127),
    C8(128),C8(129),C8(130),C8(131),C8(132),C8(133),C8(134),C8(135),
    C8(136),C8(137),C8(138),C8(139),C8(140),C8(141),C8(142),C8(143),
    C8(144),C8(145),C8(146),C8(147),C8(148),C8(149),C8(150),C8(151),
    C8(152),C8(153),C8(154),C8(155),C8(156),C8(157),C8(158),C8(159),
    C8(160),C8(161),C8(162),C8(163),C8(164),C8(165),C8(166),C8(167),
    C8(168),C8(169),C8(170),C8(171),C8(172),C8(173),C8(174),C8(175),
    C8(176),C8(177),C8(178),C8(179),C8(180),C8(181),C8(182),C8(183),
    C8(184),C8(185),C8(186),C8(187),C8(188),C8(189),C8(190),C8(191),
    C8(192),C8(193),C8(194),C8(195),C8(196),C8(197),C8(198),C8(199),
    C8(200),C8(201),C8(202),C8(203),C8(204),C8(205),C8(206),C8(207),
    C8(208),C8(209),C8(210),C8(211),C8(212),C8(213),C8(214),C8(215),
    C8(216),C8(217),C8(218),C8(219),C8(220),C8(221),C8(222),C8(223),
    C8(224),C8(225),C8(226),C8(227),C8(228),C8(229),C8(230),C8(231),
    C8(232),C8(233),C8(234),C8(235),C8(236),C8(237),C8(238),C8(239),
    C8(240),C8(241),C8(242),C8(243),C8(244),C8(245),C8(246),C8(247),
    C8(248),C8(249),C8(250),C8(251),C8(252),C8(253),C8(254),C8(255)
#undef C8
};

int irand(void)
{
    uint32_t s = (uint32_t)random_seed;
    s = s * 0x015A4E35u + 1u;
    random_seed = (long)s;
    return (int)((s >> 16) & 0x7FFF);
}

int iatan2(int y, int x)
{
    double a = atan2((double)y, (double)x);
    int r = (int)lround(a * 256.0 / (2.0 * 3.14159265358979323846));
    return r & 0xFF;
}

int isqrt(long x)
{
    if (x <= 0) return 0;
    return (int)sqrt((double)x);
}

int ihypot(int x, int y) { return isqrt((long)x * x + (long)y * y); }
} // extern "C"

// ---------------------------------------------------------------- keyboard
extern "C" {
static uint8_t g_key_groups[16] = {};
static bool g_save_menu_active = false;
static bool g_route_select_active = false;
static bool g_title_menu_active = false;
static bool g_continue_menu_active = false;

void key_start(void) {}
void key_end(void) {}
void key_beep_on(void) {}
void key_beep_off(void) {}
int key_sense(int group)
{
    return (group >= 0 && group < 16) ? g_key_groups[group] : 0;
}

int port_key_group_sense(int group)
{
    return (group >= 0 && group < 16) ? g_key_groups[group] : 0;
}
unsigned key_sense_bios(void)
{
    for (int i = 0; i < 16; i++) {
        if (g_key_groups[i]) {
            // The HIT KEY wait consumes the key that dismissed it, just like
            // the PC-98 BIOS keyboard buffer does.  Otherwise the same touch
            // would immediately select START on the title menu.
            for (int j = 0; j < 16; j++) {
                g_key_groups[j] = 0;
            }
            return 1;
        }
    }
    return 0;
}
} // extern "C"

void port_input_set_key_group(int group, uint8_t bits)
{
    if (group >= 0 && group < 16) g_key_groups[group] = bits;
}

void port_set_save_menu_active(bool active)
{
    g_save_menu_active = active;
}

void port_set_route_select_active(bool active)
{
    g_route_select_active = active;
}

void port_set_title_menu_active(bool active)
{
    g_title_menu_active = active;
}

void port_set_continue_menu_active(bool active)
{
    g_continue_menu_active = active;
}

bool port_save_menu_active(void)
{
    return g_save_menu_active || g_route_select_active ||
           g_title_menu_active || g_continue_menu_active;
}

// ---------------------------------------------------------------- graphics
extern "C" {
void graph_start(void) {}
void graph_400line(void) {}
void graph_200line(int) {}
void graph_clear(void) {}
void graph_show(void) {}
void graph_hide(void) {}
int graph_copy_page(int) { return 0; }
void graph_scrollup(unsigned) {}

void palette_show(void) {}
void palette_init(void) {}

void egc_on(void) { port_egc_set_active(true); }
void egc_off(void) { port_egc_set_active(false); }
void egc_start(void) {}

void grcg_setcolor(int, vc2) {}
void grcg_off(void) {}

void grcg_boxfill(int x1, int y1, int x2, int y2)
{
    port_fill_rect(x1, y1, x2, y2, port_grcg_color());
}

void text_clear(void)
{
    // TRAM: 80x25 cells, JIS words at A000, attributes at A200.
    uint16_t *jis = (uint16_t *)port_mk_fp(SEG_TRAM_JIS, 0);
    uint8_t *atrb = (uint8_t *)port_mk_fp(SEG_TRAM_ATRB, 0);
    for (int i = 0; i < 80 * 25; i++) {
        jis[i] = 0x0000;
        atrb[i] = 0;
    }
}

void text_fillca(unsigned ch, unsigned atrb)
{
    uint16_t *jis = (uint16_t *)port_mk_fp(SEG_TRAM_JIS, 0);
    uint8_t *a = (uint8_t *)port_mk_fp(SEG_TRAM_ATRB, 0);
    for (int i = 0; i < 80 * 25; i++) {
        jis[i] = (uint16_t)ch;
        a[i] = (uint8_t)atrb;
    }
}
} // extern "C"

// ---------------------------------------------------------------- resident
extern "C" void *resdata_exist(const char *id, unsigned idlen, unsigned parasize)
{
    (void)id; (void)idlen; (void)parasize;
    static uint8_t buffer[0x10000];
    static bool initialized = false;
    if (!initialized) {
        memset(buffer, 0, sizeof(buffer));
        initialized = true;
    }
    return buffer;
}

extern "C" void *resdata_create(const char *id, unsigned idlen, unsigned parasize)
{
    return resdata_exist(id, idlen, parasize);
}

extern "C" void dos_free(void *seg) { (void)seg; }
extern "C" void dos_puts2(const char *s) { if (s) fputs(s, stdout); }
extern "C" void dos_puts(const char *s) { if (s) fputs(s, stdout); }

// ---------------------------------------------------------------- Pi loader
extern "C" int port_pi_blit(
    void *buff, uint16_t size, int x, int y, int option,
    int (*read_func)(void *, unsigned)
);

extern "C" int PiBlitL(
    void *buff, uint16_t Size, int X, int Y, int option,
    int (pascal *read_func)(void *buf, unsigned size)
)
{
    return port_pi_blit(buff, Size, X, Y, option, read_func);
}
