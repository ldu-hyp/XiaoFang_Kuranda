#include "games/GameRegistry.hpp"

#include <array>
#include "esp_random.h"

namespace xiaofang {
namespace {

class SnakeGame final : public Game {
public:
    const char *name() const override { return "snake"; }

    esp_err_t start() override
    {
        body_.fill({0, 0});
        length_ = 3;
        body_[0] = {4, 3};
        body_[1] = {4, 4};
        body_[2] = {4, 5};
        dir_ = XF_DIR_UP;
        elapsed_ms_ = 0;
        score_ = 0;
        high_ = storage_get_high_score(XF_SCORE_SNAKE);
        over_ = false;
        spawnFood();
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (over_) {
            return;
        }

        if (input.dir_pressed && input.dir != XF_DIR_NONE && !opposite(input.dir, dir_)) {
            dir_ = input.dir;
        }

        elapsed_ms_ += dt_ms;
        uint32_t interval = 520U;
        if (score_ < 25U) {
            interval -= score_ * 14U;
        }
        if (interval < 140U) {
            interval = 140U;
        }

        if (elapsed_ms_ >= interval) {
            elapsed_ms_ = 0;
            step();
        }
    }

    void render() const override
    {
        display_clear();

        if (length_ < kMaxCells) {
            display_set_pixel(food_.x, food_.y, XF_COLOR_RED);
        }

        for (size_t i = 0; i < length_; ++i) {
            display_set_pixel(
                body_[i].x,
                body_[i].y,
                i == 0 ? XF_COLOR_GREEN : XF_COLOR_WHITE
            );
        }

        if (over_) {
            for (int i = 0; i < 8; ++i) {
                display_set_pixel(i, i, XF_COLOR_RED);
                display_set_pixel(7 - i, i, XF_COLOR_RED);
            }
        }
    }

    bool finished() const override { return over_; }

private:
    static constexpr size_t kMaxCells = 64;

    static bool same(const xf_point_t &a, const xf_point_t &b)
    {
        return a.x == b.x && a.y == b.y;
    }

    static bool opposite(xf_dir_t a, xf_dir_t b)
    {
        return (a == XF_DIR_UP && b == XF_DIR_DOWN) ||
               (a == XF_DIR_DOWN && b == XF_DIR_UP) ||
               (a == XF_DIR_LEFT && b == XF_DIR_RIGHT) ||
               (a == XF_DIR_RIGHT && b == XF_DIR_LEFT);
    }

    bool onBody(const xf_point_t &point, size_t count) const
    {
        if (count > length_) {
            count = length_;
        }
        for (size_t i = 0; i < count; ++i) {
            if (same(point, body_[i])) {
                return true;
            }
        }
        return false;
    }

    void spawnFood()
    {
        if (length_ >= kMaxCells) {
            return;
        }

        for (int tries = 0; tries < 256; ++tries) {
            xf_point_t p{
                static_cast<int16_t>(esp_random() % 8U),
                static_cast<int16_t>(esp_random() % 8U)
            };
            if (!onBody(p, length_)) {
                food_ = p;
                return;
            }
        }
    }

    xf_point_t nextHead() const
    {
        xf_point_t p = body_[0];
        switch (dir_) {
        case XF_DIR_UP:    --p.y; break;
        case XF_DIR_DOWN:  ++p.y; break;
        case XF_DIR_LEFT:  --p.x; break;
        case XF_DIR_RIGHT: ++p.x; break;
        default: break;
        }
        return p;
    }

    void finishRun()
    {
        over_ = true;
        if (score_ > high_) {
            storage_set_high_score(XF_SCORE_SNAKE, score_);
        }
    }

    void step()
    {
        if (length_ == 0 || length_ > kMaxCells) {
            finishRun();
            return;
        }

        const xf_point_t next = nextHead();
        const bool grow = same(next, food_);

        if (next.x < 0 || next.x >= 8 || next.y < 0 || next.y >= 8) {
            finishRun();
            return;
        }

        /*
         * Important fix over the Rust rewrite:
         * test NEXT head. If not growing, the current tail leaves this tick,
         * so moving into that tail cell is legal.
         */
        const size_t collision_count = grow ? length_ : length_ - 1U;
        if (onBody(next, collision_count)) {
            finishRun();
            return;
        }

        size_t last_index = length_ - 1U;
        if (grow && length_ < kMaxCells) {
            last_index = length_;
        }

        for (size_t i = last_index; i > 0; --i) {
            body_[i] = body_[i - 1U];
        }
        body_[0] = next;

        if (grow) {
            if (length_ < kMaxCells) {
                ++length_;
                ++score_;
                buzzer_score();
                spawnFood();
            } else {
                finishRun();
            }
        }
    }

    std::array<xf_point_t, kMaxCells> body_{};
    size_t length_{0};
    xf_dir_t dir_{XF_DIR_UP};
    xf_point_t food_{};
    uint32_t elapsed_ms_{0};
    uint16_t score_{0};
    uint16_t high_{0};
    bool over_{false};
};

SnakeGame g_game;

}  // namespace

Game &snakeGame()
{
    return g_game;
}

}  // namespace xiaofang
