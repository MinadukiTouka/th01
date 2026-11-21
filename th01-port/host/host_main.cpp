// Desktop test harness: runs TH01's main loop and dumps the first rendered
// frame as a PPM.  Used to validate the portable PC-98 runtime without an
// Android device.

#include "th01/core/initexit.hpp"
#include "th01/resident.hpp"
#include "th01/pc98/entry.hpp"
#include "th01/hardware/palette.h"
#include "th01/hardware/graph.h"
#include "th01/rank.h"
#include "video.h"
#include "port_time.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <chrono>

static int g_frames = 0;
static bool g_autoplay = false;
static int g_dump_frame = 240;
static char g_out_path[256] = "frame.ppm";

static void dump_ppm(const char *path, int page)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n640 400\n255\n");
    for (int y = 0; y < 400; y++) {
        for (int x = 0; x < 640; x++) {
            int off = (y * 80) + (x >> 3);
            uint8_t bit = (uint8_t)(0x80 >> (x & 7));
            int col = 0;
            if (port_video_plane(page, 0)[off] & bit) col |= 1;
            if (port_video_plane(page, 1)[off] & bit) col |= 2;
            if (port_video_plane(page, 2)[off] & bit) col |= 4;
            if (port_video_plane(page, 3)[off] & bit) col |= 8;
            uint8_t r = (uint8_t)(z_Palettes[col].v[0] * 17);
            uint8_t g = (uint8_t)(z_Palettes[col].v[1] * 17);
            uint8_t b = (uint8_t)(z_Palettes[col].v[2] * 17);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    }
    fclose(f);
}

static void on_frame(void)
{
    g_frames++;
    if (g_autoplay) {
        if (g_frames == 240) port_input_set_key_group(5, 0x02);
        if (g_frames == 242) port_input_set_key_group(5, 0);
        if (g_frames >= 300 && g_frames < 340) port_input_set_key_group(7, 0x10);
        else if (g_frames >= 340) port_input_set_key_group(7, 0);
    }
    if (g_frames == g_dump_frame) {
        dump_ppm(g_out_path, port_video_show_page());
        dump_ppm("frame_p1.ppm", 1);
        fprintf(stderr, "dumped %s after %d frames\n", g_out_path, g_frames);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    if (argc < 2) {
        fprintf(stderr, "usage: %s <th01-data-dir>\n", argv[0]);
        return 2;
    }
    if (chdir(argv[1]) != 0) {
        perror("chdir");
        return 2;
    }
    int stage = (argc > 2) ? atoi(argv[2]) : 0;
    int route_arg = (argc > 3) ? atoi(argv[3]) : 1;
    if (stage < 0 || stage >= 20) stage = 0;
    if (argc > 4) g_dump_frame = atoi(argv[4]);
    snprintf(g_out_path, sizeof(g_out_path), "frame_s%02d_r%d.ppm", stage, route_arg);

    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(45));
        fprintf(stderr, "watchdog timeout\n");
        _Exit(0);
    }).detach();

    fprintf(stderr, "resident... stage=%d route=%d\n", stage, route_arg);
    resident = resident_get_or_create();
    resident->route = (route_arg == 0) ? ROUTE_MAKAI : ROUTE_JIGOKU;
    resident->rank = RANK_NORMAL;
    resident->stage_id = stage;
    resident->bgm_mode = BGM_MODE_OFF;
    resident->rem_lives = 4;
    resident->rem_bombs = 3;

    g_autoplay = getenv("TH01_AUTOPLAY") != nullptr;
    port_set_frame_callback(on_frame);
    fprintf(stderr, "game_init...\n");
    game_init();
    fprintf(stderr, "main_main...\n");
    main_main(0, nullptr);
    fprintf(stderr, "main_main returned\n");
    return 0;
}
