#pragma once

#include <cstdint>
#include "esp_err.h"
#include "c_api.hpp"
#include "games/GameManager.hpp"
#include "ui/Ui.hpp"

namespace xiaofang {

class Application final {
public:
    esp_err_t init();
    [[noreturn]] void run();

private:
    enum class State : uint8_t {
        Menu,
        Game,
        Temperature,
    };

    static bool menuToGame(ui::Item item, GameId &id);
    void enterMenu();
    void handleMenu(const xf_input_t &input);

    State state_{State::Menu};
    ui::Item menu_{ui::Item::Hourglass};
    xf_settings_t settings_{};
    int64_t temperature_until_us_{0};
    GameManager games_{};
};

}  // namespace xiaofang
