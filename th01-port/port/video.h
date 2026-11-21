#pragma once
#include <stdint.h>

uint8_t *port_video_plane(int page, int plane);
int port_video_access_page(void);
void port_video_set_access_page(int page);
int port_video_show_page(void);
void port_video_set_show_page(int page);
void port_video_refresh_plane_ptrs(void);
