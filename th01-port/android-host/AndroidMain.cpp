// Android host for the TH01 port: SDL2 window + RGBA texture + touch/key
// mapping around the portable PC-98 runtime.

#include <SDL.h>
#include <jni.h>

#include "th01/core/initexit.hpp"
#include "th01/resident.hpp"
#include "th01/pc98/entry.hpp"
#include "th01/hardware/palette.h"
#include "th01/rank.h"
#include "video.h"
#include "port_time.h"
#include "input.h"
#include "audio.h"
#include "gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static SDL_Window *g_window = nullptr;
static SDL_Renderer *g_renderer = nullptr;
static SDL_Texture *g_texture = nullptr;
static uint32_t g_rgba[640 * 400];
static SDL_AudioDeviceID g_audio = 0;
static int g_have_freq = 0, g_have_ch = 0, g_have_samples = 0, g_have_fmt = 0;
static bool g_audio_swap = false;

static void audio_callback(void *, Uint8 *stream, int len)
{
    port_audio_render(reinterpret_cast<int16_t *>(stream), len / 4);
    if (g_audio_swap) {
        for (int i = 0; i + 1 < len; i += 2) {
            Uint8 t = stream[i];
            stream[i] = stream[i + 1];
            stream[i + 1] = t;
        }
    }
}

static void set_key(int group, uint8_t bits)
{
    port_input_set_key_group(group, bits);
}

extern bool input_bomb;

extern "C" JNIEXPORT void JNICALL
Java_com_zun_th01_GameActivity_nativeKey(JNIEnv *, jclass, jint group, jint bits)
{
    port_input_set_key_group((int)group, (uint8_t)bits);
}

extern "C" JNIEXPORT void JNICALL
Java_com_zun_th01_GameActivity_nativeBomb(JNIEnv *, jclass)
{
    input_bomb = true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_zun_th01_GameActivity_nativeSaveMenuActive(JNIEnv *, jclass)
{
    return port_save_menu_active() ? JNI_TRUE : JNI_FALSE;
}

static void handle_event(const SDL_Event &e)
{
    switch (e.type) {
    case SDL_QUIT:
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        int down = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.sym) {
        case SDLK_LEFT:  set_key(7, down ? 0x08 : 0); break;
        case SDLK_RIGHT: set_key(7, down ? 0x10 : 0); break;
        case SDLK_UP:    set_key(7, down ? 0x04 : 0); break;
        case SDLK_DOWN:  set_key(7, down ? 0x20 : 0); break;
        case SDLK_z:     set_key(5, down ? 0x02 : 0); break;
        case SDLK_x:     set_key(5, down ? 0x04 : 0); break;
        case SDLK_ESCAPE:set_key(0, down ? 0x01 : 0); break;
        case SDLK_RETURN:set_key(3, down ? 0x10 : 0); break;
        default: break;
        }
        break;
    }
    default:
        break;
    }
}

static void present_frame()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) handle_event(e);

    int page = port_video_show_page();
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
            g_rgba[y * 640 + x] = (uint32_t)(0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
        }
    }
    SDL_UpdateTexture(g_texture, nullptr, g_rgba, 640 * 4);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, nullptr, nullptr);
    SDL_RenderPresent(g_renderer);
}

static void fatal(const char *msg)
{
    SDL_Log("th01: %s", msg);
    fprintf(stderr, "th01: %s\n", msg);
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fatal(SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    g_window = SDL_CreateWindow("TH01", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                640, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!g_window) { fatal(SDL_GetError()); return 1; }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) { fatal(SDL_GetError()); return 1; }
    SDL_RenderSetLogicalSize(g_renderer, 640, 480);
    g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, 640, 400);
    if (!g_texture) { fatal(SDL_GetError()); return 1; }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 2048;
    want.callback = audio_callback;
    g_audio = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    g_have_freq = have.freq; g_have_ch = have.channels; g_have_samples = have.samples; g_have_fmt = (int)have.format;
    g_audio_swap = (have.format == AUDIO_S16MSB) || (have.format == AUDIO_U16MSB);
    if (g_audio) SDL_PauseAudioDevice(g_audio, 0);

    const char *base = SDL_AndroidGetExternalStoragePath();
    if (!base) base = SDL_AndroidGetInternalStoragePath();
    if (!base) { fatal("no storage path"); return 1; }
    char data_dir[1024];
    snprintf(data_dir, sizeof(data_dir), "%s/th01", base);
    mkdir(data_dir, 0777);
    if (chdir(data_dir) != 0) { fatal("cannot enter data dir"); return 1; }

    resident = resident_get_or_create();
    resident->route = ROUTE_MAKAI;
    resident->rank = RANK_NORMAL;
    resident->stage_id = 0;
    resident->bgm_mode = BGM_MODE_MDRV2;
    resident->snd_need_init = true;
    resident->rem_lives = 4;
    resident->rem_bombs = 3;

    port_set_frame_callback(present_frame);

    int entry = EP_OP;
    for (;;) {
        if (entry == EP_OP) {
            main_op(0, nullptr);
        } else if (entry == EP_MAIN) {
            main_main(0, nullptr);
        } else {
            break;
        }

        entrypoint_t next;
        if (!port_entrypoint_pending(&next)) {
            break;
        }
        entry = (next == EP_MAIN) ? EP_MAIN : EP_OP;
    }

    SDL_DestroyTexture(g_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
