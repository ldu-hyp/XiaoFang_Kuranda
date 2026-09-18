#include "games/GameRegistry.hpp"

#include "esp_random.h"

namespace xiaofang {
namespace {

class DiceGame final : public Game {
public:
    const char *name() const override { return "dice"; }

    esp_err_t start() override
    {
        value_ = 1;
        rolling_ms_ = 0;
        frame_ms_ = 0;
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (input.shake) {
            rolling_ms_ = 650;
            frame_ms_ = 0;
        }

        if (rolling_ms_ == 0) {
            return;
        }

        rolling_ms_ = dt_ms >= rolling_ms_ ? 0U : rolling_ms_ - dt_ms;
        frame_ms_ += dt_ms;
        if (frame_ms_ >= 70U) {
            frame_ms_ = 0;
            value_ = static_cast<uint8_t>((esp_random() % 6U) + 1U);
        }
    }

    void render() const override
    {
        display_clear();
        const uint8_t v = value_;

        if (v == 1 || v == 3 || v == 5) {
            pip(3, 3);
        }
        if (v >= 2) {
            pip(1, 1);
            pip(5, 5);
        }
        if (v >= 4) {
            pip(5, 1);
            pip(1, 5);
        }
        if (v == 6) {
            pip(1, 3);
            pip(5, 3);
        }
    }

    bool finished() const override { return false; }

private:
    static void pip(int x, int y)
    {
        display_set_pixel(x, y, XF_COLOR_RED);
        display_set_pixel(x + 1, y, XF_COLOR_ORANGE);
    }

    uint8_t value_{1};
    uint32_t rolling_ms_{0};
    uint32_t frame_ms_{0};
};

DiceGame g_game;

}  // namespace

Game &diceGame()
{
    return g_game;
}

}  // namespace xiaofang
