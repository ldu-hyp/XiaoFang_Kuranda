#include "games/GameRegistry.hpp"

#include <array>
#include "esp_random.h"

namespace xiaofang {
namespace {

class BaguaGame final : public Game {
public:
    const char *name() const override { return "bagua"; }

    esp_err_t start() override
    {
        value_ = 0;
        rolling_ms_ = 0;
        frame_ms_ = 0;
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (input.shake) {
            rolling_ms_ = 700;
            frame_ms_ = 0;
        }

        if (rolling_ms_ == 0) {
            return;
        }

        rolling_ms_ = dt_ms >= rolling_ms_ ? 0U : rolling_ms_ - dt_ms;
        frame_ms_ += dt_ms;
        if (frame_ms_ >= 80U) {
            frame_ms_ = 0;
            value_ = static_cast<uint8_t>(esp_random() % kTrigrams.size());
        }
    }

    void render() const override
    {
        display_clear();
        const uint8_t trigram = kTrigrams[value_ & 7U];

        for (int line = 0; line < 3; ++line) {
            const int y = 1 + line * 2;
            const bool solid = ((trigram >> (2 - line)) & 1U) != 0U;
            const xf_rgb_t color =
                line == 0 ? XF_COLOR_YELLOW :
                line == 1 ? XF_COLOR_ORANGE :
                            XF_COLOR_WHITE;

            if (solid) {
                for (int x = 0; x < 8; ++x) {
                    display_set_pixel(x, y, color);
                }
            } else {
                for (int x = 0; x < 3; ++x) {
                    display_set_pixel(x, y, color);
                }
                for (int x = 5; x < 8; ++x) {
                    display_set_pixel(x, y, color);
                }
            }

            for (int x = 0; x < 8; ++x) {
                display_set_pixel(x, y + 1, display_get_pixel(x, y));
            }
        }
    }

    bool finished() const override { return false; }

private:
    static constexpr std::array<uint8_t, 8> kTrigrams{{
        0b111, 0b000, 0b001, 0b100, 0b101, 0b010, 0b110, 0b011
    }};

    uint8_t value_{0};
    uint32_t rolling_ms_{0};
    uint32_t frame_ms_{0};
};

BaguaGame g_game;

}  // namespace

Game &baguaGame()
{
    return g_game;
}

}  // namespace xiaofang
