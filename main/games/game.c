#include "game.h"
#include "game_internal.h"

static const game_module_t *s_active;
static game_id_t s_id = GAME_COUNT;

static const game_module_t *module_for(game_id_t id)
{
    switch (id) {
    case GAME_HOURGLASS: return game_hourglass_module();
    case GAME_DICE: return game_dice_module();
    case GAME_BAGUA: return game_bagua_module();
    case GAME_SNAKE: return game_snake_module();
    case GAME_MAZE: return game_maze_module();
    case GAME_CUBE_MAN: return game_cube_man_module();
    case GAME_SOKOBAN: return game_sokoban_module();
    case GAME_DODGE: return game_dodge_module();
    case GAME_PONG: return game_pong_module();
    default: return NULL;
    }
}

esp_err_t game_start(game_id_t id)
{
    game_stop();
    s_active = module_for(id);
    if (!s_active) return ESP_ERR_INVALID_ARG;
    s_id = id;
    return s_active->start ? s_active->start() : ESP_OK;
}

void game_update(const xf_input_t *input, uint32_t dt_ms)
{
    if (s_active && s_active->update) s_active->update(input, dt_ms);
}

void game_render(void)
{
    if (s_active && s_active->render) s_active->render();
}

bool game_finished(void)
{
    return !s_active || (s_active->finished && s_active->finished());
}

void game_stop(void)
{
    if (s_active && s_active->stop) s_active->stop();
    s_active = NULL;
    s_id = GAME_COUNT;
}

game_id_t game_active_id(void) { return s_id; }
