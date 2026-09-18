#pragma once

#include <cstdint>
#include "esp_err.h"
#include "c_api.hpp"

namespace xiaofang {

enum class GameId : uint8_t {
    Hourglass = 0,
    Dice,
    Bagua,
    Snake,
    Maze,
    CubeMan,
    Sokoban,
    Dodge,
    Pong,
    Count,
};

class Game {
public:
    virtual ~Game() = default;

    virtual const char *name() const = 0;
    virtual esp_err_t start() = 0;
    virtual void update(const xf_input_t &input, uint32_t dt_ms) = 0;
    virtual void render() const = 0;
    virtual bool finished() const = 0;
    virtual void stop() {}
};

}  // namespace xiaofang
