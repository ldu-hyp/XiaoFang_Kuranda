#include "games/GameRegistry.hpp"

#include <cstring>

namespace xiaofang {
namespace {

constexpr size_t kLevelCount = 3;

constexpr const char *kLevels[kLevelCount][8] = {
    {
        "########",
        "#      #",
        "# $.@  #",
        "#      #",
        "#      #",
        "#      #",
        "#      #",
        "########",
    },
    {
        "########",
        "#  .   #",
        "#  $   #",
        "# .$@  #",
        "#      #",
        "#      #",
        "#      #",
        "########",
    },
    {
        "########",
        "# .  . #",
        "# $$   #",
        "#   #  #",
        "# @    #",
        "#      #",
        "#      #",
        "########",
    },
};

class SokobanGame final : public Game {
public:
    const char *name() const override { return "sokoban"; }

    esp_err_t start() override
    {
        done_ = false;
        loadLevel(0);
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t) override
    {
        if (done_ || !input.dir_pressed || input.dir == XF_DIR_NONE) {
            return;
        }

        int dx = 0;
        int dy = 0;
        switch (input.dir) {
        case XF_DIR_UP:    dy = -1; break;
        case XF_DIR_DOWN:  dy = 1; break;
        case XF_DIR_LEFT:  dx = -1; break;
        case XF_DIR_RIGHT: dx = 1; break;
        default: return;
        }

        const int nx = player_.x + dx;
        const int ny = player_.y + dy;

        if (!inside(nx, ny) || wall_[ny][nx]) {
            return;
        }

        if (box_[ny][nx]) {
            const int bx = nx + dx;
            const int by = ny + dy;
            if (!inside(bx, by) || wall_[by][bx] || box_[by][bx]) {
                return;
            }

            box_[ny][nx] = false;
            box_[by][bx] = true;
        }

        player_ = {
            static_cast<int16_t>(nx),
            static_cast<int16_t>(ny)
        };

        if (complete()) {
            buzzer_score();
            const size_t next_level = level_ + 1U;
            if (next_level >= kLevelCount) {
                done_ = true;
            } else {
                loadLevel(next_level);
            }
        }
    }

    void render() const override
    {
        display_clear();

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (wall_[y][x]) {
                    display_set_pixel(x, y, xf_rgb(45, 45, 60));
                } else if (goal_[y][x]) {
                    display_set_pixel(x, y, XF_COLOR_GREEN);
                }

                if (box_[y][x]) {
                    display_set_pixel(
                        x, y,
                        goal_[y][x] ? XF_COLOR_CYAN : XF_COLOR_BLUE
                    );
                }
            }
        }

        display_set_pixel(
            player_.x,
            player_.y,
            goal_[player_.y][player_.x] ? XF_COLOR_YELLOW : XF_COLOR_RED
        );
    }

    bool finished() const override { return done_; }
    GameResult result() const override
    {
        return done_ ? GameResult::Success : GameResult::Running;
    }

private:
    static bool inside(int x, int y)
    {
        return x >= 0 && x < 8 && y >= 0 && y < 8;
    }

    void loadLevel(size_t level)
    {
        std::memset(wall_, 0, sizeof(wall_));
        std::memset(goal_, 0, sizeof(goal_));
        std::memset(box_, 0, sizeof(box_));
        level_ = level;

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const char c = kLevels[level][y][x];
                switch (c) {
                case '#': wall_[y][x] = true; break;
                case '.': goal_[y][x] = true; break;
                case '$': box_[y][x] = true; break;
                case '@': player_ = {static_cast<int16_t>(x), static_cast<int16_t>(y)}; break;
                case '*':
                    box_[y][x] = true;
                    goal_[y][x] = true;
                    break;
                case '+':
                    player_ = {static_cast<int16_t>(x), static_cast<int16_t>(y)};
                    goal_[y][x] = true;
                    break;
                default:
                    break;
                }
            }
        }
    }

    bool complete() const
    {
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (goal_[y][x] && !box_[y][x]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool wall_[8][8]{};
    bool goal_[8][8]{};
    bool box_[8][8]{};
    xf_point_t player_{};
    size_t level_{0};
    bool done_{false};
};

SokobanGame g_game;

}  // namespace

Game &sokobanGame()
{
    return g_game;
}

}  // namespace xiaofang
