#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// BGM/SE hooks implemented by port/audio.cpp (or the OGG backend later).
void port_audio_bgm_load(const char *fn);
void port_audio_bgm_play(void);
void port_audio_bgm_stop(void);
void port_audio_bgm_fade_out(void);
void port_audio_se_load(const char *fn);
void port_audio_se_play(int se);
void port_audio_update(void);
int port_audio_sample_rate(void);
void port_audio_render(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif
