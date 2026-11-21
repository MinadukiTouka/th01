// Offline OGG BGM backend.
//
// The Android APK bundles OGG rips in bgm/<track>.ogg.  The game still calls
// the old MDRV2 API with .MDT filenames; this layer maps each filename to the
// corresponding OGG track.
//
// Tracks are decoded once on the game thread and the audio callback only
// copies integer samples.  Decoding inside the callback caused underruns on
// slower devices, which sounded like rapid clicking/"machine gun" noise.

#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mutex>
#include <string>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

namespace {

std::mutex g_lock;
int16_t *g_pcm = nullptr;
size_t g_samples = 0;
size_t g_pos = 0;
int g_rate = 44100;
int g_channels = 2;
bool g_playing = false;
bool g_loaded = false;
std::string g_current;

struct SEBankEntry {
    int16_t *pcm = nullptr;
    int frames = 0;
    int channels = 2;
};

SEBankEntry g_se_bank[17];

struct SEVoice {
    bool active = false;
    const int16_t *pcm = nullptr;
    int frames = 0;
    int channels = 2;
    int pos = 0;
};
SEVoice g_se[8];

std::string lower_basename(const char *fn)
{
    std::string s = fn ? fn : "";
    size_t slash = s.find_last_of("/\\");
    if (slash != std::string::npos) s = s.substr(slash + 1);
    size_t dot = s.find_last_of('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// Map the original MDRV2 filename to the 01..16 OGG rip number.
int track_for(const std::string &base)
{
    if (base.size() == 3 && base[0] == 's' && base[1] == 't' &&
        base[2] >= '0' && base[2] <= '6') {
        static const int st[] = { 2, 4, 5, 7, 8, 10, 2 };
        return st[base[2] - '0'];
    }
    if (base == "positive") return 3;
    if (base == "legend") return 6;
    if (base == "kami") return 9;
    if (base == "kami2") return 16;
    if (base == "tensi") return 11;
    if (base == "alice") return 13;
    if (base == "reimu") return 1;
    if (base == "iris") return 14;
    if (base == "syugen") return 12;
    if (base == "zipangu") return 15;
    if (base == "shugen") return 13;
    if (base == "init") return 1;
    return 1;
}

void free_track()
{
    if (g_pcm) {
        free(g_pcm);
        g_pcm = nullptr;
    }
    g_samples = 0;
    g_pos = 0;
    g_loaded = false;
    g_playing = false;
    g_current.clear();
}

} // namespace

extern "C" {

void port_audio_bgm_load(const char *fn)
{
    std::lock_guard<std::mutex> lock(g_lock);

    int track = track_for(lower_basename(fn));
    char path_buf[128];
    snprintf(path_buf, sizeof(path_buf), "bgm/%02d.ogg", track);
    std::string path = path_buf;

    // The original MDRV2 driver ignored repeated loads of the same file.
    // main_01.cpp calls this every frame from the life loop, so reloading
    // here would restart the track constantly ("machine gun" stutter).
    if (g_loaded && g_current == path) return;

    free_track();

    int channels = 0;
    int rate = 0;
    short *samples = nullptr;
    int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &samples);
    if (frames <= 0 || !samples) {
        std::string alt = std::string("bgm/") + lower_basename(fn) + ".ogg";
        frames = stb_vorbis_decode_filename(alt.c_str(), &channels, &rate, &samples);
        if (frames > 0 && samples) path = alt;
    }
    if (frames <= 0 || !samples) {
        if (samples) free(samples);
        return;
    }

    g_pcm = (int16_t *)samples;
    g_samples = (size_t)frames * (size_t)channels;
    g_rate = rate > 0 ? rate : 44100;
    g_channels = channels > 0 ? channels : 2;
    g_current = path;
    g_loaded = true;
}

void port_audio_bgm_play(void)
{
    std::lock_guard<std::mutex> lock(g_lock);
    if (!g_loaded) return;
    g_playing = true;
    if (g_pos >= g_samples) g_pos = 0;
}

void port_audio_bgm_stop(void)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_playing = false;
    g_pos = 0;
}

void port_audio_bgm_fade_out(void)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_playing = false;
}

void port_audio_se_load(const char *fn)
{
    (void)fn;
    std::lock_guard<std::mutex> lock(g_lock);
    for (auto &e : g_se_bank) {
        if (e.pcm) {
            free(e.pcm);
            e.pcm = nullptr;
        }
        e.frames = 0;
        e.channels = 2;
    }
    for (int i = 1; i <= 17; i++) {
        char path[32];
        snprintf(path, sizeof(path), "se/%02d.ogg", i);
        int channels = 0;
        int rate = 0;
        short *samples = nullptr;
        int frames = stb_vorbis_decode_filename(path, &channels, &rate, &samples);
        if (frames <= 0 || !samples) {
            if (samples) free(samples);
            continue;
        }
        g_se_bank[i - 1].pcm = (int16_t *)samples;
        g_se_bank[i - 1].frames = frames;
        g_se_bank[i - 1].channels = channels > 0 ? channels : 2;
    }
}

void port_audio_se_play(int se)
{
    if (se <= 0 || se > 17) return;
    std::lock_guard<std::mutex> lock(g_lock);
    SEBankEntry &bank = g_se_bank[se - 1];
    if (!bank.pcm || bank.frames <= 0) return;

    SEVoice *v = nullptr;
    for (auto &c : g_se) {
        if (!c.active) { v = &c; break; }
    }
    if (!v) {
        v = &g_se[0];
        for (auto &c : g_se) if (c.pos > v->pos) v = &c;
    }
    v->active = true;
    v->pcm = bank.pcm;
    v->frames = bank.frames;
    v->channels = bank.channels;
    v->pos = 0;
}

void port_audio_update(void)
{
}

int port_audio_sample_rate(void)
{
    return g_rate;
}

// Called from the SDL audio callback.  Pure sample copy, no decoding.
void port_audio_render(int16_t *out, int frames)
{
    std::lock_guard<std::mutex> lock(g_lock);
    memset(out, 0, sizeof(int16_t) * 2 * (size_t)frames);

    if (g_playing && g_pcm) {
        for (int i = 0; i < frames; i++) {
            if (g_pos + (size_t)g_channels > g_samples) g_pos = 0;
            int16_t l = 0;
            int16_t r = 0;
            if (g_channels == 1) {
                l = r = g_pcm[g_pos++];
            } else {
                l = g_pcm[g_pos++];
                r = g_pcm[g_pos++];
            }
            for (int c = 2; c < g_channels; c++) g_pos++;
            out[i * 2] = l;
            out[i * 2 + 1] = r;
        }
    }

    // Pre-rendered SE samples (dev-time OGG -> decoded PCM).
    for (auto &v : g_se) {
        if (!v.active || !v.pcm) continue;
        for (int i = 0; i < frames && v.pos < v.frames; i++) {
            int16_t l = v.pcm[v.pos * v.channels];
            int16_t r = (v.channels > 1)
                ? v.pcm[v.pos * v.channels + 1]
                : l;
            v.pos++;
            int32_t il = out[i * 2] + l;
            int32_t ir = out[i * 2 + 1] + r;
            if (il > 32767) il = 32767;
            if (il < -32768) il = -32768;
            if (ir > 32767) ir = 32767;
            if (ir < -32768) ir = -32768;
            out[i * 2] = (int16_t)il;
            out[i * 2 + 1] = (int16_t)ir;
        }
        if (v.pos >= v.frames) v.active = false;
    }
}

} // extern "C"
