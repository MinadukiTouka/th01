// Silent audio backend stub.  Replaced by the OGG/SDL2 backend in a later
// milestone; keeping the symbols here lets the game logic link and run.

#include "audio.h"
#include <stdio.h>

extern "C" {

void port_audio_bgm_load(const char *fn) { (void)fn; }
void port_audio_bgm_play(void) {}
void port_audio_bgm_stop(void) {}
void port_audio_bgm_fade_out(void) {}
void port_audio_se_load(const char *fn) { (void)fn; }
void port_audio_se_play(int se) { (void)se; }
void port_audio_update(void) {}

} // extern "C"
