#include "games/GameManager.hpp"
#include "games/GameRegistry.hpp"

namespace xiaofang {

Game *GameManager::resolve(GameId id)
{
    switch (id) {
    case GameId::Hourglass: return &hourglassGame();
    case GameId::Dice:      return &diceGame();
    case GameId::Bagua:     return &baguaGame();
    case GameId::Snake:     return &snakeGame();
    case GameId::Maze:      return &mazeGame();
    case GameId::CubeMan:   return &cubeManGame();
    case GameId::Sokoban:   return &sokobanGame();
    case GameId::Dodge:     return &dodgeGame();
    case GameId::Pong:      return &pongGame();
    default:
        return nullptr;
    }
}

esp_err_t GameManager::start(GameId id)
{
    stop();

    active_ = resolve(id);
    if (active_ == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    active_id_ = id;
    const esp_err_t err = active_->start();
    if (err != ESP_OK) {
        active_->stop();
        active_ = nullptr;
        active_id_ = GameId::Count;
    }
    return err;
}

void GameManager::update(const xf_input_t &input, uint32_t dt_ms)
{
    if (active_ != nullptr) {
        active_->update(input, dt_ms);
    }
}

void GameManager::render() const
{
    if (active_ != nullptr) {
        active_->render();
    }
}

bool GameManager::finished() const
{
    return result() != GameResult::Running;
}

GameResult GameManager::result() const
{
    return active_ != nullptr
        ? active_->result()
        : GameResult::Failure;
}

void GameManager::stop()
{
    if (active_ != nullptr) {
        active_->stop();
    }
    active_ = nullptr;
    active_id_ = GameId::Count;
}

}  // namespace xiaofang
