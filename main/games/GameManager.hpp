#pragma once

#include "games/Game.hpp"

namespace xiaofang {

class GameManager final {
public:
    esp_err_t start(GameId id);
    void update(const xf_input_t &input, uint32_t dt_ms);
    void render() const;
    bool finished() const;
    void stop();

    GameId activeId() const { return active_id_; }
    bool active() const { return active_ != nullptr; }

private:
    static Game *resolve(GameId id);

    Game *active_{nullptr};
    GameId active_id_{GameId::Count};
};

}  // namespace xiaofang
