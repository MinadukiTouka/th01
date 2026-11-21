/* ReC98
 * -----
 * MDRV2 functions - portable Android backend.
 *
 * The original implementation called the MDRV98.COM DOS TSR via INT F2h.
 * The port keeps the same game-facing API, but forwards to the audio layer
 * (offline OGG playback, or a future native MDRV2 player).
 */

#include "platform.h"
#include "th01/snd/mdrv2.h"
#include "audio.h"

bool16 mdrv2_resident(void)
{
    return true;
}

bool16 mdrv2_enable_if_board_installed(void)
{
    return true;
}

void mdrv2_bgm_load(const char *fn)
{
    port_audio_bgm_load(fn);
}

void mdrv2_se_load(const char *fn)
{
    port_audio_se_load(fn);
}

void mdrv2_bgm_play(void)
{
    port_audio_bgm_play();
}

void mdrv2_bgm_stop(void)
{
    port_audio_bgm_stop();
}

void mdrv2_bgm_fade_out_nonblock(void)
{
    port_audio_bgm_fade_out();
}

void mdrv2_bgm_fade_out_block(void)
{
    port_audio_bgm_fade_out();
}

void mdrv2_bgm_fade_in(void)
{
}

void mdrv2_se_play(int se)
{
    if (se) port_audio_se_play(se);
}
