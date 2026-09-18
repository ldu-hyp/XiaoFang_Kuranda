#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "xf_types.h"

esp_err_t display_init(void);
void display_deinit(void);

void display_clear(void);
void display_fill(xf_rgb_t color);
void display_set_pixel(int x, int y, xf_rgb_t color);
xf_rgb_t display_get_pixel(int x, int y);
void display_draw_mono_bitmap(const uint8_t rows[8], xf_rgb_t on, xf_rgb_t off);
void display_set_brightness(uint8_t brightness);
uint8_t display_get_brightness(void);
void display_set_rotation(uint8_t quarter_turns);
esp_err_t display_show(void);
void display_off(void);

extern const xf_rgb_t XF_COLOR_BLACK;
extern const xf_rgb_t XF_COLOR_WHITE;
extern const xf_rgb_t XF_COLOR_RED;
extern const xf_rgb_t XF_COLOR_GREEN;
extern const xf_rgb_t XF_COLOR_BLUE;
extern const xf_rgb_t XF_COLOR_YELLOW;
extern const xf_rgb_t XF_COLOR_CYAN;
extern const xf_rgb_t XF_COLOR_ORANGE;
extern const xf_rgb_t XF_COLOR_PURPLE;
extern const xf_rgb_t XF_COLOR_DIM;
