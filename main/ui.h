#pragma once

#include <stddef.h>
#include <stdint.h>
#include "xf_types.h"

typedef enum {
    UI_ITEM_HOURGLASS = 0,
    UI_ITEM_DICE,
    UI_ITEM_BAGUA,
    UI_ITEM_SNAKE,
    UI_ITEM_MAZE,
    UI_ITEM_CUBE_MAN,
    UI_ITEM_SOKOBAN,
    UI_ITEM_DODGE,
    UI_ITEM_PONG,
    UI_ITEM_TEMPERATURE,
    UI_ITEM_SOUND,
    UI_ITEM_COUNT,
} ui_item_t;

void ui_draw_menu(ui_item_t item);
void ui_draw_number(int value, xf_rgb_t color);
void ui_draw_face_wakeup(void);
const char *ui_item_name(ui_item_t item);
