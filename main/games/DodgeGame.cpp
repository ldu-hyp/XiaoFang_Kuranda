#include "games/GameRegistry.hpp"

#include <array>
#include "esp_random.h"

namespace xiaofang {
namespace {

class DodgeGame final : public Game {
public:
    const char *name() const override { return "dodge"; }

    esp_err_t start() override
    {
        rows_.fill(0);
        player_ = {3, 7};
        elapsed_ms_ = 0;
        score_ = 0;
        high_ = storage_get_high_score(XF_SCORE_DODGE);
        over_ = false;
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (over_) {
            return;
        }

        if (input.dir_changed) {
            if (input.dir == XF_DIR_LEFT && player_.x > 0) --player_.x;
            else if (input.dir == XF_DIR_RIGHT && player_.x < 7) ++player_.x;
            else if (input.dir == XF_DIR_UP && player_.y > 3) --player_.y;
            else if (input.dir == XF_DIR_DOWN && player_.y < 7) ++player_.y;
        }

        elapsed_ms_ += dt_ms;
        uint32_t interval = 500U - (score_ < 40U ? score_ * 7U : 280U);
        if (interval < 180U) {
            interval = 180U;
        }
        if (elapsed_ms_ < interval) {
            return;
        }
        elapsed_ms_ = 0;

        /* row 0 is the top; obstacles move downward toward the player. */
        for (size_t y = 7; y > 0; --y) {
            rows_[y] = rows_[y - 1U];
        }
        rows_[0] = newObstacleRow();

        if ((rows_[static_cast<size_t>(player_.y)] & (1U << player_.x)) != 0U) {
            over_ = true;
            if (score_ > high_) {
                storage_set_high_score(XF_SCORE_DODGE, score_);
            }
            return;
        }

        ++score_;
        if ((score_ % 10U) == 0U) {
            buzzer_score();
        }
    }

    void render() const override
    {
        display_clear();

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if ((rows_[static_cast<size_t>(y)] & (1U << x)) != 0U) {
                    display_set_pixel(x, y, XF_COLOR_CYAN);
                }
            }
        }

        display_set_pixel(
            player_.x,
            player_.y,
            over_ ? XF_COLOR_RED : XF_COLOR_ORANGE
        );
    }

    bool finished() const override { return over_; }

private:
    uint8_t newObstacleRow() const
    {
        if ((esp_random() % 3U) == 0U) {
            return 0;
        }

        const int gap = score_ < 12U ? 2 : 1;
        const int start = static_cast<int>(esp_random() % static_cast<uint32_t>(9 - gap));
        uint8_t row = 0xff;
        for (int i = 0; i < gap; ++i) {
            row &= static_cast<uint8_t>(~(1U << (start + i)));
        }
        return row;
    }

    std::array<uint8_t, 8> rows_{};
    xf_point_t player_{3, 7};
    uint32_t elapsed_ms_{0};
    uint16_t score_{0};
    uint16_t high_{0};
    bool over_{false};
};

DodgeGame g_game;

}  // namespace

Game &dodgeGame()
{
    return g_game;
}

}  // namespace xiaofang
