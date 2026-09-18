#include "ui.h"

#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const uint8_t ICONS[UI_ITEM_COUNT][8] = {
    [UI_ITEM_HOURGLASS]   = {0x00,0x7e,0x3c,0x18,0x18,0x3c,0x7e,0x00},
    [UI_ITEM_DICE]        = {0x00,0x66,0x66,0x18,0x18,0x66,0x66,0x00},
    [UI_ITEM_BAGUA]       = {0xff,0xff,0x00,0xe7,0xe7,0x00,0xff,0xff},
    [UI_ITEM_SNAKE]       = {0x00,0x74,0x40,0x7e,0x02,0x1e,0x10,0x00},
    [UI_ITEM_MAZE]        = {0x00,0x56,0x5a,0x42,0x3a,0x22,0x6e,0x00},
    [UI_ITEM_CUBE_MAN]    = {0x00,0x1c,0x00,0x0f,0x00,0xf0,0x00,0x3e},
    [UI_ITEM_SOKOBAN]     = {0x66,0xbd,0x81,0xbd,0x81,0xdb,0x42,0x7e},
    [UI_ITEM_DODGE]       = {0xff,0xad,0xed,0x8d,0xad,0xbe,0x00,0x10},
    [UI_ITEM_PONG]        = {0x81,0x81,0x01,0x18,0x18,0x80,0x81,0x81},
    [UI_ITEM_TEMPERATURE] = {0x18,0x18,0x18,0x18,0x3c,0x7e,0x3c,0x00},
    [UI_ITEM_SOUND]       = {0x08,0x18,0x38,0x78,0x78,0x38,0x18,0x08},
};

static const char *NAMES[UI_ITEM_COUNT] = {
    "hourglass","dice","bagua","snake","maze","cube_man",
    "sokoban","dodge","pong","temperature","sound"
};

static xf_rgb_t item_color(ui_item_t item)
{
    static const xf_rgb_t colors[] = {
        {255,150,0},{255,40,40},{220,180,30},{40,255,80},{80,140,255},
        {255,80,20},{0,220,255},{180,0,255},{255,30,30},{0,220,180},{100,120,255}
    };
    return colors[item % UI_ITEM_COUNT];
}

void ui_draw_menu(ui_item_t item)
{
    display_draw_mono_bitmap(ICONS[item], item_color(item), XF_COLOR_BLACK);
}

const char *ui_item_name(ui_item_t item)
{
    return (unsigned)item < UI_ITEM_COUNT ? NAMES[item] : "unknown";
}

static const uint8_t DIGIT[10][5] = {
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},
};

static void draw_digit(int digit, int ox, xf_rgb_t color)
{
    if (digit < 0 || digit > 9) return;
    for (int y=0; y<5; ++y) {
        for (int x=0; x<3; ++x) {
            if (DIGIT[digit][y] & (1U << (2-x))) display_set_pixel(ox+x, y+1, color);
        }
    }
}

void ui_draw_number(int value, xf_rgb_t color)
{
    display_clear();
    if (value < 0) {
        display_set_pixel(0,3,color);
        display_set_pixel(1,3,color);
        value = -value;
    }
    value %= 100;
    if (value >= 10) {
        draw_digit(value / 10, 0, color);
        draw_digit(value % 10, 4, color);
    } else {
        draw_digit(value, 2, color);
    }
}

void ui_draw_face_wakeup(void)
{
    static const uint8_t face[] = {
        0x00,0x66,0x66,0x00,0x00,0x24,0x18,0x00
    };
    display_draw_mono_bitmap(face, XF_COLOR_CYAN, XF_COLOR_BLACK);
    display_show();
    vTaskDelay(pdMS_TO_TICKS(350));
}
