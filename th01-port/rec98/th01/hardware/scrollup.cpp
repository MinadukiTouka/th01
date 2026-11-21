#include "x86real.h"
#include "th01/hardware/scrollup.hpp"
#include "port_time.h"

// The PC-98 GDC scroll is not emulated; advancing a frame is enough for all
// gameplay logic that waits for the scroll to finish.
void z_vsync_wait_and_scrollup(vram_y_t line_on_top)
{
    (void)line_on_top;
    port_frame_delay(1);
}
