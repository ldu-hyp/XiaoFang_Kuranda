#include "games/GameRegistry.hpp"

#include <array>
#include <cstring>
#include "esp_random.h"

namespace xiaofang {
namespace {

class MazeGame final : public Game {
public:
    const char *name() const override { return "maze"; }

    esp_err_t start() override
    {
        wins_ = 0;
        generate();
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t) override
    {
        if (!input.dir_changed || input.dir == XF_DIR_NONE) {
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
        if (nx < 0 || ny < 0 || nx >= kWidth || ny >= kHeight || wall_[ny][nx]) {
            return;
        }

        player_ = {
            static_cast<int16_t>(nx),
            static_cast<int16_t>(ny)
        };

        if (player_.x == goal_.x && player_.y == goal_.y) {
            ++wins_;
            buzzer_score();
            generate();
        }
    }

    void render() const override
    {
        int vx = player_.x - 3;
        int vy = player_.y - 3;

        if (vx < 0) vx = 0;
        if (vy < 0) vy = 0;
        if (vx > kWidth - 8) vx = kWidth - 8;
        if (vy > kHeight - 8) vy = kHeight - 8;

        display_clear();

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const int gx = vx + x;
                const int gy = vy + y;
                if (wall_[gy][gx]) {
                    display_set_pixel(x, y, xf_rgb(35, 45, 70));
                }
            }
        }

        const int gx = goal_.x - vx;
        const int gy = goal_.y - vy;
        if (gx >= 0 && gx < 8 && gy >= 0 && gy < 8) {
            display_set_pixel(gx, gy, XF_COLOR_GREEN);
        }

        display_set_pixel(player_.x - vx, player_.y - vy, XF_COLOR_RED);
    }

    bool finished() const override { return false; }

private:
    static constexpr int kWidth = 21;
    static constexpr int kHeight = 21;
    static constexpr int kCellCount = kWidth * kHeight;

    static void shuffle4(std::array<uint8_t, 4> &dirs)
    {
        for (int i = 3; i > 0; --i) {
            const int j = static_cast<int>(esp_random() % static_cast<uint32_t>(i + 1));
            const uint8_t t = dirs[static_cast<size_t>(i)];
            dirs[static_cast<size_t>(i)] = dirs[static_cast<size_t>(j)];
            dirs[static_cast<size_t>(j)] = t;
        }
    }

    void generate()
    {
        std::memset(wall_, 1, sizeof(wall_));
        player_ = {1, 1};
        wall_[1][1] = false;

        int top = 0;
        work_[static_cast<size_t>(top++)] = player_;

        constexpr int dx[4] = {0, 2, 0, -2};
        constexpr int dy[4] = {-2, 0, 2, 0};

        while (top > 0) {
            const xf_point_t current = work_[static_cast<size_t>(top - 1)];
            std::array<uint8_t, 4> dirs{{0, 1, 2, 3}};
            shuffle4(dirs);
            bool carved = false;

            for (uint8_t d : dirs) {
                const int nx = current.x + dx[d];
                const int ny = current.y + dy[d];

                if (nx <= 0 || ny <= 0 || nx >= kWidth - 1 || ny >= kHeight - 1) {
                    continue;
                }
                if (!wall_[ny][nx]) {
                    continue;
                }

                wall_[current.y + dy[d] / 2][current.x + dx[d] / 2] = false;
                wall_[ny][nx] = false;
                work_[static_cast<size_t>(top++)] = {
                    static_cast<int16_t>(nx),
                    static_cast<int16_t>(ny)
                };
                carved = true;
                break;
            }

            if (!carved) {
                --top;
            }
        }

        for (auto &row : distance_) {
            for (auto &value : row) {
                value = -1;
            }
        }

        int head = 0;
        int tail = 0;
        work_[static_cast<size_t>(tail++)] = player_;
        distance_[player_.y][player_.x] = 0;
        goal_ = player_;

        constexpr int sx[4] = {0, 1, 0, -1};
        constexpr int sy[4] = {-1, 0, 1, 0};

        while (head < tail) {
            const xf_point_t p = work_[static_cast<size_t>(head++)];

            if (distance_[p.y][p.x] > distance_[goal_.y][goal_.x]) {
                goal_ = p;
            }

            for (int d = 0; d < 4; ++d) {
                const int nx = p.x + sx[d];
                const int ny = p.y + sy[d];

                if (nx < 0 || ny < 0 || nx >= kWidth || ny >= kHeight ||
                    wall_[ny][nx] || distance_[ny][nx] >= 0) {
                    continue;
                }

                distance_[ny][nx] = static_cast<int16_t>(distance_[p.y][p.x] + 1);
                work_[static_cast<size_t>(tail++)] = {
                    static_cast<int16_t>(nx),
                    static_cast<int16_t>(ny)
                };
            }
        }
    }

    bool wall_[kHeight][kWidth]{};
    int16_t distance_[kHeight][kWidth]{};
    std::array<xf_point_t, kCellCount> work_{};
    xf_point_t player_{1, 1};
    xf_point_t goal_{1, 1};
    uint16_t wins_{0};
};

MazeGame g_game;

}  // namespace

Game &mazeGame()
{
    return g_game;
}

}  // namespace xiaofang
