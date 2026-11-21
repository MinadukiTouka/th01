#pragma once
#include <stdint.h>

uint8_t port_grcg_color(void);
void port_grcg_set_color(uint8_t col);
void port_grcg_set_tile_color(uint8_t col);
void port_fill_rect(int x1, int y1, int x2, int y2, uint8_t col);
