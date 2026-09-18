#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} xf_rgb_t;

typedef struct {
    int16_t x;
    int16_t y;
} xf_point_t;

typedef enum {
    XF_DIR_NONE = 0,
    XF_DIR_UP,
    XF_DIR_RIGHT,
    XF_DIR_DOWN,
    XF_DIR_LEFT,
} xf_dir_t;

typedef struct {
    xf_dir_t dir;

    /* Compatibility edge flag: true whenever the classified direction changes. */
    bool dir_changed;

    /* Key-like direction events for deterministic game controls. */
    bool dir_pressed;
    bool dir_repeat;
    bool dir_released;

    bool shake;
    bool pause;
    bool face_down;
    bool activity;
    float temperature_c;
} xf_input_t;

typedef enum {
    XF_SCORE_SNAKE = 0,
    XF_SCORE_CUBE_MAN,
    XF_SCORE_DODGE,
    XF_SCORE_PONG,
    XF_SCORE_COUNT,
} xf_score_slot_t;

static inline xf_rgb_t xf_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (xf_rgb_t){ .r = r, .g = g, .b = b };
}
