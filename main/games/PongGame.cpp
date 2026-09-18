#include "games/GameRegistry.hpp"

#include <array>
#include <cstring>
#include "esp_random.h"
#include "esp_timer.h"

namespace xiaofang {
namespace {

class PongGame final : public Game {
public:
    const char *name() const override { return "pong"; }

    esp_err_t start() override
    {
        resetState();

        const esp_err_t err = network_init();
        if (err != ESP_OK) {
            return err;
        }

        network_get_mac(self_mac_.data());
        started_us_ = esp_timer_get_time();
        last_rx_us_ = started_us_;
        return ESP_OK;
    }

    void update(const xf_input_t &input, uint32_t dt_ms) override
    {
        if (over_) {
            return;
        }

        if (input.dir_changed) {
            if (input.dir == XF_DIR_UP && my_paddle_ > 0) {
                --my_paddle_;
            } else if (input.dir == XF_DIR_DOWN && my_paddle_ < 6) {
                ++my_paddle_;
            }
        }

        xf_net_packet_t packet{};
        while (network_poll(&packet)) {
            handlePacket(packet);
        }

        const int64_t now = esp_timer_get_time();

        if (!paired_) {
            if (role_ == Role::Unknown && (now - started_us_) > 1'000'000) {
                role_ = Role::Host;
            }

            if (role_ == Role::Host) {
                seek_ms_ += dt_ms;
                if (seek_ms_ >= 200U) {
                    seek_ms_ = 0;
                    const uint8_t seek[2] = {kProtocolCode, kSeek};
                    network_broadcast(seek, sizeof(seek));
                }
            }

            if ((now - started_us_) > 8'000'000) {
                over_ = true;
            }
            return;
        }

        if ((now - last_rx_us_) > 1'000'000) {
            over_ = true;
            return;
        }

        elapsed_ms_ += dt_ms;
        if (elapsed_ms_ < 50U) {
            return;
        }
        elapsed_ms_ = 0;

        if (role_ == Role::Host) {
            simulateHost();
            if (!over_) {
                sendState();
            }
        } else {
            sendPaddle();
        }
    }

    void render() const override
    {
        display_clear();

        if (!paired_) {
            const uint32_t phase = static_cast<uint32_t>((esp_timer_get_time() / 120000) % 6);
            display_set_pixel(0, 3, XF_COLOR_WHITE);
            display_set_pixel(0, 4, XF_COLOR_WHITE);
            display_set_pixel(7, 3, XF_COLOR_WHITE);
            display_set_pixel(7, 4, XF_COLOR_WHITE);
            display_set_pixel(1 + static_cast<int>(phase), 3, XF_COLOR_RED);
            return;
        }

        int ball_x = ball_x8_ / 8;
        const int ball_y = ball_y8_ / 8;

        /*
         * Network coordinates are always Host-world coordinates.
         * Client mirrors X so each player still sees themselves on the left.
         */
        if (role_ == Role::Client) {
            ball_x = 7 - ball_x;
        }

        display_set_pixel(0, my_paddle_, XF_COLOR_WHITE);
        display_set_pixel(0, my_paddle_ + 1, XF_COLOR_WHITE);
        display_set_pixel(7, peer_paddle_, XF_COLOR_WHITE);
        display_set_pixel(7, peer_paddle_ + 1, XF_COLOR_WHITE);

        if (ball_x >= 0 && ball_x < 8 && ball_y >= 0 && ball_y < 8) {
            display_set_pixel(ball_x, ball_y, XF_COLOR_RED);
        }
    }

    bool finished() const override { return over_; }

    void stop() override
    {
        if (network_is_ready()) {
            network_shutdown();
        }
    }

private:
    enum class Role : uint8_t {
        Unknown = 0,
        Host,
        Client,
    };

    static constexpr uint8_t kProtocolCode = 1;
    static constexpr uint8_t kEnd = 0;
    static constexpr uint8_t kSeek = 1;
    static constexpr uint8_t kJoin = 2;
    static constexpr uint8_t kGame = 3;

    static bool sameMac(const uint8_t *a, const std::array<uint8_t, 6> &b)
    {
        return std::memcmp(a, b.data(), b.size()) == 0;
    }

    void resetState()
    {
        role_ = Role::Unknown;
        self_mac_.fill(0);
        peer_.fill(0);
        my_paddle_ = 3;
        peer_paddle_ = 3;
        ball_x8_ = 28;
        ball_y8_ = 28;
        vx8_ = 2;
        vy8_ = 1;
        my_score_ = 0;
        peer_score_ = 0;
        elapsed_ms_ = 0;
        seek_ms_ = 0;
        started_us_ = 0;
        last_rx_us_ = 0;
        paired_ = false;
        over_ = false;
    }

    void serve()
    {
        ball_x8_ = 28;
        ball_y8_ = 28;
        vx8_ = static_cast<int8_t>(((esp_random() & 1U) != 0U ? 1 : -1) * 2);
        vy8_ = static_cast<int8_t>((esp_random() & 1U) != 0U ? 1 : -1);
    }

    void sendJoin()
    {
        const uint8_t payload[2] = {kProtocolCode, kJoin};
        network_send(peer_.data(), payload, sizeof(payload));
    }

    void sendState()
    {
        const uint8_t payload[8] = {
            kProtocolCode,
            kGame,
            my_paddle_,
            peer_paddle_,
            static_cast<uint8_t>(ball_x8_ / 8),
            static_cast<uint8_t>(ball_y8_ / 8),
            my_score_,
            peer_score_,
        };
        network_send(peer_.data(), payload, sizeof(payload));
    }

    void sendPaddle()
    {
        const uint8_t payload[3] = {kProtocolCode, kGame, my_paddle_};
        network_send(peer_.data(), payload, sizeof(payload));
    }

    void handlePacket(const xf_net_packet_t &packet)
    {
        if (packet.len < 2 ||
            packet.data[0] != kProtocolCode ||
            sameMac(packet.src, self_mac_)) {
            return;
        }

        if (!paired_ && packet.data[1] == kSeek) {
            if (role_ == Role::Unknown ||
                (role_ == Role::Host &&
                 std::memcmp(self_mac_.data(), packet.src, self_mac_.size()) > 0)) {
                role_ = Role::Client;
                std::memcpy(peer_.data(), packet.src, peer_.size());
                sendJoin();
                paired_ = true;
                last_rx_us_ = esp_timer_get_time();
                buzzer_menu_enter();
            }
            return;
        }

        if (!paired_ && packet.data[1] == kJoin && role_ == Role::Host) {
            std::memcpy(peer_.data(), packet.src, peer_.size());
            paired_ = true;
            last_rx_us_ = esp_timer_get_time();
            serve();
            sendState();
            buzzer_menu_enter();
            return;
        }

        if (!paired_ || !sameMac(packet.src, peer_)) {
            return;
        }

        last_rx_us_ = esp_timer_get_time();

        if (packet.data[1] == kEnd) {
            over_ = true;
            return;
        }
        if (packet.data[1] != kGame) {
            return;
        }

        if (role_ == Role::Host && packet.len >= 3) {
            peer_paddle_ = packet.data[2];
        } else if (role_ == Role::Client && packet.len >= 8) {
            peer_paddle_ = packet.data[2];
            ball_x8_ = static_cast<int16_t>(packet.data[4] * 8);
            ball_y8_ = static_cast<int16_t>(packet.data[5] * 8);
            peer_score_ = packet.data[6];
            my_score_ = packet.data[7];

            if (my_score_ >= 5 || peer_score_ >= 5) {
                over_ = true;
            }
        }
    }

    void simulateHost()
    {
        ball_x8_ += vx8_;
        ball_y8_ += vy8_;

        if (ball_y8_ < 0) {
            ball_y8_ = 0;
            vy8_ = static_cast<int8_t>(-vy8_);
        }
        if (ball_y8_ > 56) {
            ball_y8_ = 56;
            vy8_ = static_cast<int8_t>(-vy8_);
        }

        const int x = ball_x8_ / 8;
        const int y = ball_y8_ / 8;

        if (vx8_ < 0 && x <= 0 &&
            (y == my_paddle_ || y == my_paddle_ + 1)) {
            ball_x8_ = 8;
            vx8_ = static_cast<int8_t>(-vx8_);
            buzzer_tone(3200, 20);
        } else if (vx8_ > 0 && x >= 7 &&
                   (y == peer_paddle_ || y == peer_paddle_ + 1)) {
            ball_x8_ = 48;
            vx8_ = static_cast<int8_t>(-vx8_);
            buzzer_tone(3200, 20);
        }

        if (ball_x8_ < 0) {
            ++peer_score_;
            buzzer_score();
            serve();
        } else if (ball_x8_ > 56) {
            ++my_score_;
            buzzer_score();
            serve();
        }

        if (my_score_ >= 5 || peer_score_ >= 5) {
            const uint8_t payload[4] = {kProtocolCode, kEnd, my_score_, peer_score_};
            network_send(peer_.data(), payload, sizeof(payload));
            over_ = true;

            const uint16_t high = storage_get_high_score(XF_SCORE_PONG);
            if (my_score_ > high) {
                storage_set_high_score(XF_SCORE_PONG, my_score_);
            }
        }
    }

    Role role_{Role::Unknown};
    std::array<uint8_t, 6> self_mac_{};
    std::array<uint8_t, 6> peer_{};
    uint8_t my_paddle_{3};
    uint8_t peer_paddle_{3};
    int16_t ball_x8_{28};
    int16_t ball_y8_{28};
    int8_t vx8_{2};
    int8_t vy8_{1};
    uint8_t my_score_{0};
    uint8_t peer_score_{0};
    uint32_t elapsed_ms_{0};
    uint32_t seek_ms_{0};
    int64_t started_us_{0};
    int64_t last_rx_us_{0};
    bool paired_{false};
    bool over_{false};
};

PongGame g_game;

}  // namespace

Game &pongGame()
{
    return g_game;
}

}  // namespace xiaofang
