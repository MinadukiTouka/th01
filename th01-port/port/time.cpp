#include "platform.h"
#include "th01/hardware/vsync.hpp"
#include "port_time.h"

#include <unistd.h>

static void (*g_frame_callback)(void) = nullptr;

void port_set_frame_callback(void (*cb)(void))
{
    g_frame_callback = cb;
}

// PC-98 native frame period: 1 / 56.423 Hz ≈ 17.723 ms.
void port_frame_delay(unsigned int frames)
{
    for (unsigned int i = 0; i < frames; i++) {
        usleep(17723);
        z_vsync_Count1++;
        z_vsync_Count2++;
        if (g_frame_callback) g_frame_callback();
    }
}
