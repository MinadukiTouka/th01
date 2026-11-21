#include "platform.h"
#include "th01/hardware/frmdelay.h"
#include "port_time.h"

void frame_delay(unsigned int frames)
{
    port_frame_delay(frames);
}
