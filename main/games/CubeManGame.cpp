#include "games/GameRegistry.hpp"

#include <array>
#include "esp_random.h"

namespace xiaofang {
namespace {

class CubeManGame final : public Game {
public:
    const char *name() const override { return "cube_man"; }

    esp_err_t start() override
    {
        masks_.fill(0);
        types_.fill(FloorType::None);
        player_ = {3, 5};
        masks_[6] = 0x1c;
        types_[6] = FloorType::Normal;
        masks_[7] = 0x7e;
        types_[7] = FloorType::Normal;
        elapsed_ms_ = 0;
        score_ = 0;
        high_ = storage_get_high_score(XF_SCORE_CUBE_MAN);
        over_ = false;
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (over_) {
            return;
        }

        if (input.dir_changed) {
            if (input.dir == XF_DIR_LEFT && player_.x > 0) {
                --player_.x;
            } else if (input.dir == XF_DIR_RIGHT && player_.x < 7) {
                ++player_.x;
            }
        }

        elapsed_ms_ += dt_ms;
        uint32_t interval = 260U - (score_ < 90U ? score_ * 2U : 180U);
        if (interval < 80U) {
            interval = 80U;
        }
        if (elapsed_ms_ < interval) {
            return;
        }
        elapsed_ms_ = 0;

        for (size_t y = 0; y < 7; ++y) {
            masks_[y] = masks_[y + 1U];
            types_[y] = types_[y + 1U];
        }
        makeFloor(7);

        const int below = player_.y + 1;
        const bool supported =
            below >= 0 && below < 8 &&
            (masks_[static_cast<size_t>(below)] & (1U << player_.x)) != 0U;

        if (supported) {
            const FloorType type = types_[static_cast<size_t>(below)];
            --player_.y;
            ++score_;

            if ((score_ % 10U) == 0U) {
                buzzer_score();
            }

            switch (type) {
            case FloorType::Fragile:
                masks_[static_cast<size_t>(below)] &= static_cast<uint8_t>(~(1U << player_.x));
                break;
            case FloorType::ConveyorLeft:
                if (player_.x > 0) --player_.x;
                break;
            case FloorType::ConveyorRight:
                if (player_.x < 7) ++player_.x;
                break;
            case FloorType::Spring:
                player_.y -= 2;
                break;
            default:
                break;
            }
        } else {
            ++player_.y;
        }

        if (player_.y < 0 || player_.y >= 8) {
            over_ = true;
            if (score_ > high_) {
                storage_set_high_score(XF_SCORE_CUBE_MAN, score_);
            }
        }
    }

    void render() const override
    {
        display_clear();

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if ((masks_[static_cast<size_t>(y)] & (1U << x)) != 0U) {
                    display_set_pixel(x, y, floorColor(types_[static_cast<size_t>(y)]));
                }
            }
        }

        if (player_.y >= 0 && player_.y < 8) {
            display_set_pixel(
                player_.x,
                player_.y,
                over_ ? XF_COLOR_RED : XF_COLOR_ORANGE
            );
        }
    }

    bool finished() const override { return over_; }

private:
    enum class FloorType : uint8_t {
        None = 0,
        Normal,
        Fragile,
        ConveyorLeft,
        ConveyorRight,
        Spring,
    };

    void makeFloor(size_t y)
    {
        masks_[y] = 0;
        types_[y] = FloorType::None;

        if ((esp_random() % 100U) < 35U) {
            return;
        }

        const int len = 3 + static_cast<int>(esp_random() % 3U);
        const int start = static_cast<int>(esp_random() % static_cast<uint32_t>(9 - len));
        for (int x = start; x < start + len; ++x) {
            masks_[y] |= static_cast<uint8_t>(1U << x);
        }

        const uint32_t r = esp_random() % 10U;
        if (r < 7U) {
            types_[y] = FloorType::Normal;
        } else if (r == 7U) {
            types_[y] = FloorType::Fragile;
        } else if (r == 8U) {
            types_[y] = (esp_random() & 1U) != 0U
                ? FloorType::ConveyorLeft
                : FloorType::ConveyorRight;
        } else {
            types_[y] = FloorType::Spring;
        }
    }

    static xf_rgb_t floorColor(FloorType type)
    {
        switch (type) {
        case FloorType::Fragile:       return xf_rgb(100, 100, 100);
        case FloorType::ConveyorLeft:
        case FloorType::ConveyorRight: return XF_COLOR_GREEN;
        case FloorType::Spring:        return XF_COLOR_YELLOW;
        case FloorType::Normal:        return XF_COLOR_WHITE;
        default:                       return XF_COLOR_BLACK;
        }
    }

    std::array<uint8_t, 8> masks_{};
    std::array<FloorType, 8> types_{};
    xf_point_t player_{3, 5};
    uint32_t elapsed_ms_{0};
    uint16_t score_{0};
    uint16_t high_{0};
    bool over_{false};
};

CubeManGame g_game;

}  // namespace

Game &cubeManGame()
{
    return g_game;
}

}  // namespace xiaofang
