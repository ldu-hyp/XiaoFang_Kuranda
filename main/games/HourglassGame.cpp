#include "games/GameRegistry.hpp"

#include <cstring>
#include "esp_random.h"

namespace xiaofang {
namespace {

class HourglassGame final : public Game {
public:
    const char *name() const override { return "hourglass"; }

    esp_err_t start() override
    {
        reset();
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (complete_) {
            alarm_ms_ += dt_ms;
            if (input.activity) {
                reset();
                return;
            }
            if ((alarm_ms_ % 1000U) < dt_ms) {
                buzzer_tone(3000, 45);
            }
            return;
        }

        elapsed_ms_ += dt_ms;
        if (elapsed_ms_ >= 180U) {
            elapsed_ms_ = 0;
            fallStep();
        }
    }

    void render() const override
    {
        display_clear();
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (!allowed(x, y)) {
                    continue;
                }
                display_set_pixel(
                    x, y,
                    sand_[y][x] ? XF_COLOR_YELLOW : xf_rgb(8, 12, 18)
                );
            }
        }

        if (complete_ && (((alarm_ms_ / 250U) & 1U) != 0U)) {
            display_set_pixel(3, 3, XF_COLOR_RED);
            display_set_pixel(4, 3, XF_COLOR_RED);
            display_set_pixel(3, 4, XF_COLOR_RED);
            display_set_pixel(4, 4, XF_COLOR_RED);
        }
    }

    bool finished() const override { return false; }

private:
    static bool allowed(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 8 || y >= 8) {
            return false;
        }
        const int inset = y < 4 ? y : 7 - y;
        return x >= inset && x <= 7 - inset;
    }

    void reset()
    {
        std::memset(sand_, 0, sizeof(sand_));
        elapsed_ms_ = 0;
        alarm_ms_ = 0;
        complete_ = false;

        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (allowed(x, y)) {
                    sand_[y][x] = true;
                }
            }
        }
    }

    bool hasUpperSand() const
    {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (sand_[y][x]) {
                    return true;
                }
            }
        }
        return false;
    }

    void fallStep()
    {
        bool moved = false;

        for (int y = 6; y >= 0; --y) {
            const bool reverse = (esp_random() & 1U) != 0U;
            for (int n = 0; n < 8; ++n) {
                const int x = reverse ? 7 - n : n;
                if (!sand_[y][x]) {
                    continue;
                }

                int options[3] = {x, x - 1, x + 1};
                if ((esp_random() & 1U) != 0U) {
                    const int tmp = options[1];
                    options[1] = options[2];
                    options[2] = tmp;
                }

                for (int nx : options) {
                    const int ny = y + 1;
                    if (allowed(nx, ny) && !sand_[ny][nx]) {
                        sand_[y][x] = false;
                        sand_[ny][nx] = true;
                        moved = true;
                        break;
                    }
                }
            }
        }

        if (!hasUpperSand()) {
            complete_ = true;
            alarm_ms_ = 0;
            return;
        }

        if (!moved) {
            for (int y = 2; y >= 0; --y) {
                for (int x = 0; x < 8; ++x) {
                    if (sand_[y][x] && allowed(3, y + 1) && !sand_[y + 1][3]) {
                        sand_[y][x] = false;
                        sand_[y + 1][3] = true;
                        return;
                    }
                }
            }
        }
    }

    bool sand_[8][8]{};
    uint32_t elapsed_ms_{0};
    uint32_t alarm_ms_{0};
    bool complete_{false};
};

HourglassGame g_game;

}  // namespace

Game &hourglassGame()
{
    return g_game;
}

}  // namespace xiaofang
