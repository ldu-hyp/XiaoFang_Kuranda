#include "app/Application.hpp"

#include <cmath>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace xiaofang {
namespace {
constexpr const char *TAG = "app";
}

bool Application::menuToGame(ui::Item item, GameId &id)
{
    switch (item) {
    case ui::Item::Hourglass: id = GameId::Hourglass; return true;
    case ui::Item::Dice:      id = GameId::Dice; return true;
    case ui::Item::Bagua:     id = GameId::Bagua; return true;
    case ui::Item::Snake:     id = GameId::Snake; return true;
    case ui::Item::Maze:      id = GameId::Maze; return true;
    case ui::Item::CubeMan:   id = GameId::CubeMan; return true;
    case ui::Item::Sokoban:   id = GameId::Sokoban; return true;
    case ui::Item::Dodge:     id = GameId::Dodge; return true;
    case ui::Item::Pong:      id = GameId::Pong; return true;
    default:
        return false;
    }
}

esp_err_t Application::init()
{
    ESP_RETURN_ON_ERROR(storage_init(), TAG, "storage");
    ESP_RETURN_ON_ERROR(storage_load_settings(&settings_), TAG, "settings");

    ESP_RETURN_ON_ERROR(display_init(), TAG, "display");
    display_set_brightness(settings_.brightness);

    ESP_RETURN_ON_ERROR(buzzer_init(), TAG, "buzzer");
    buzzer_set_enabled(settings_.sound_enabled);

    ESP_RETURN_ON_ERROR(imu_init(), TAG, "imu");
    ESP_RETURN_ON_ERROR(input_init(), TAG, "input");

    power_init(static_cast<uint32_t>(settings_.sleep_timeout_s) * 1000U);

    ui::drawWakeFace();
    state_ = State::Menu;
    menu_ = ui::Item::Hourglass;
    ui::drawMenu(menu_);
    ESP_RETURN_ON_ERROR(display_show(), TAG, "initial frame");

    ESP_LOGI(TAG,
             "XiaoFang Kuranda %s / ESP32-WROOM-32D ready",
             XF_FW_VERSION);
    return ESP_OK;
}

void Application::enterMenu()
{
    games_.stop();
    state_ = State::Menu;
    ui::drawMenu(menu_);
    (void)display_show();
}

void Application::beginGameResult(GameResult result)
{
    switch (result) {
    case GameResult::Success:
        buzzer_success();
        break;
    case GameResult::Failure:
        buzzer_game_over();
        break;
    case GameResult::Timeout:
    case GameResult::Disconnected:
        buzzer_warning();
        break;
    case GameResult::Running:
        return;
    }

    state_ = State::GameResult;
    result_until_us_ = esp_timer_get_time() + 450000;
}

void Application::handleMenu(const xf_input_t &input)
{
    if (input.dir_pressed && input.dir == XF_DIR_RIGHT) {
        menu_ = ui::next(menu_);
        buzzer_menu_move();
        ui::drawMenu(menu_);
        (void)display_show();
        return;
    }

    if (input.dir_pressed && input.dir == XF_DIR_LEFT) {
        menu_ = ui::previous(menu_);
        buzzer_menu_move();
        ui::drawMenu(menu_);
        (void)display_show();
        return;
    }

    if (!input.dir_pressed || input.dir != XF_DIR_UP) {
        return;
    }

    buzzer_menu_enter();

    GameId id{};
    if (menuToGame(menu_, id)) {
        if (games_.start(id) == ESP_OK) {
            state_ = State::Game;
        }
        return;
    }

    if (menu_ == ui::Item::Temperature) {
        ui::drawNumber(
            static_cast<int>(std::lround(input.temperature_c)),
            XF_COLOR_ORANGE
        );
        (void)display_show();
        temperature_until_us_ = esp_timer_get_time() + 3000000;
        state_ = State::Temperature;
        return;
    }

    if (menu_ == ui::Item::Sound) {
        settings_.sound_enabled = !settings_.sound_enabled;
        buzzer_set_enabled(settings_.sound_enabled);
        (void)storage_save_settings(&settings_);

        display_fill(
            settings_.sound_enabled ? XF_COLOR_GREEN : XF_COLOR_RED
        );
        (void)display_show();
        vTaskDelay(pdMS_TO_TICKS(120));

        ui::drawMenu(menu_);
        (void)display_show();
    }
}

[[noreturn]] void Application::run()
{
    TickType_t last = xTaskGetTickCount();

    while (true) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(XF_APP_TICK_MS));

        xf_input_t input{};
        if (input_poll(&input) != ESP_OK) {
            continue;
        }

        if (input.activity) {
            power_note_activity();
        }

        switch (state_) {
        case State::Menu:
            handleMenu(input);
            break;

        case State::Game:
            if (input.pause) {
                enterMenu();
                break;
            }

            games_.update(input, XF_APP_TICK_MS);
            games_.render();
            (void)display_show();

            if (games_.result() != GameResult::Running) {
                beginGameResult(games_.result());
            }
            break;

        case State::GameResult:
            if (esp_timer_get_time() >= result_until_us_) {
                enterMenu();
            }
            break;

        case State::Temperature:
            if (esp_timer_get_time() >= temperature_until_us_) {
                enterMenu();
            }
            break;
        }

        if (input.face_down && state_ != State::Menu) {
            (void)power_enter_deep_sleep();
        }

        if (power_should_sleep()) {
            (void)power_enter_deep_sleep();
        }
    }
}

}  // namespace xiaofang
