#include "ui/Ui.hpp"

#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace xiaofang::ui {
namespace {

constexpr size_t kItemCount = static_cast<size_t>(Item::Count);

constexpr std::array<std::array<uint8_t, 8>, kItemCount> kIcons{{
    {{0x00,0x7e,0x3c,0x18,0x18,0x3c,0x7e,0x00}},
    {{0x00,0x66,0x66,0x18,0x18,0x66,0x66,0x00}},
    {{0xff,0xff,0x00,0xe7,0xe7,0x00,0xff,0xff}},
    {{0x00,0x74,0x40,0x7e,0x02,0x1e,0x10,0x00}},
    {{0x00,0x56,0x5a,0x42,0x3a,0x22,0x6e,0x00}},
    {{0x00,0x1c,0x00,0x0f,0x00,0xf0,0x00,0x3e}},
    {{0x66,0xbd,0x81,0xbd,0x81,0xdb,0x42,0x7e}},
    {{0xff,0xad,0xed,0x8d,0xad,0xbe,0x00,0x10}},
    {{0x81,0x81,0x01,0x18,0x18,0x80,0x81,0x81}},
    {{0x18,0x18,0x18,0x18,0x3c,0x7e,0x3c,0x00}},
    {{0x08,0x18,0x38,0x78,0x78,0x38,0x18,0x08}},
}};

constexpr std::array<const char *, kItemCount> kNames{{
    "hourglass", "dice", "bagua", "snake", "maze", "cube_man",
    "sokoban", "dodge", "pong", "temperature", "sound"
}};

constexpr std::array<xf_rgb_t, kItemCount> kColors{{
    {255,150,0}, {255,40,40}, {220,180,30}, {40,255,80}, {80,140,255},
    {255,80,20}, {0,220,255}, {180,0,255}, {255,30,30}, {0,220,180},
    {100,120,255}
}};

constexpr uint8_t kDigits[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7},
};

void drawDigit(int digit, int ox, xf_rgb_t color)
{
    if (digit < 0 || digit > 9) {
        return;
    }

    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 3; ++x) {
            if ((kDigits[digit][y] & (1U << (2 - x))) != 0U) {
                display_set_pixel(ox + x, y + 1, color);
            }
        }
    }
}

}  // namespace

Item next(Item item)
{
    const auto value = (static_cast<uint8_t>(item) + 1U) % static_cast<uint8_t>(Item::Count);
    return static_cast<Item>(value);
}

Item previous(Item item)
{
    const auto count = static_cast<uint8_t>(Item::Count);
    const auto value = (static_cast<uint8_t>(item) + count - 1U) % count;
    return static_cast<Item>(value);
}

const char *name(Item item)
{
    const size_t index = static_cast<size_t>(item);
    return index < kNames.size() ? kNames[index] : "unknown";
}

void drawMenu(Item item)
{
    const size_t index = static_cast<size_t>(item);
    if (index >= kIcons.size()) {
        return;
    }
    display_draw_mono_bitmap(kIcons[index].data(), kColors[index], XF_COLOR_BLACK);
}

void drawNumber(int value, xf_rgb_t color)
{
    display_clear();

    if (value < 0) {
        display_set_pixel(0, 3, color);
        display_set_pixel(1, 3, color);
        value = -value;
    }

    value %= 100;
    if (value >= 10) {
        drawDigit(value / 10, 0, color);
        drawDigit(value % 10, 4, color);
    } else {
        drawDigit(value, 2, color);
    }
}

void drawWakeFace()
{
    constexpr uint8_t kFace[8] = {
        0x00,0x66,0x66,0x00,0x00,0x24,0x18,0x00
    };
    display_draw_mono_bitmap(kFace, XF_COLOR_CYAN, XF_COLOR_BLACK);
    display_show();
    vTaskDelay(pdMS_TO_TICKS(350));
}

}  // namespace xiaofang::ui
