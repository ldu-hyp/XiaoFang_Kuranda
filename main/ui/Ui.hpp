#pragma once

#include <cstdint>
#include "c_api.hpp"

namespace xiaofang::ui {

enum class Item : uint8_t {
    Hourglass = 0,
    Dice,
    Bagua,
    Snake,
    Maze,
    CubeMan,
    Sokoban,
    Dodge,
    Pong,
    Temperature,
    Sound,
    Count,
};

Item next(Item item);
Item previous(Item item);
const char *name(Item item);

void drawMenu(Item item);
void drawNumber(int value, xf_rgb_t color);
void drawWakeFace();

}  // namespace xiaofang::ui
