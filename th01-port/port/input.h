#pragma once
#include <stdint.h>

void port_input_set_key_group(int group, uint8_t bits);
void port_set_save_menu_active(bool active);
void port_set_route_select_active(bool active);
void port_set_title_menu_active(bool active);
void port_set_continue_menu_active(bool active);
bool port_save_menu_active(void);
