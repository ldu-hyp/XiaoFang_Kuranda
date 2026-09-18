#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "xf_types.h"

typedef enum {
    GAME_HOURGLASS = 0,
    GAME_DICE,
    GAME_BAGUA,
    GAME_SNAKE,
    GAME_MAZE,
    GAME_CUBE_MAN,
    GAME_SOKOBAN,
    GAME_DODGE,
    GAME_PONG,
    GAME_COUNT,
} game_id_t;

typedef struct game_module {
    const char *name;
    esp_err_t (*start)(void);
    void (*update)(const xf_input_t *input, uint32_t dt_ms);
    void (*render)(void);
    bool (*finished)(void);
    void (*stop)(void);
} game_module_t;

esp_err_t game_start(game_id_t id);
void game_update(const xf_input_t *input, uint32_t dt_ms);
void game_render(void);
bool game_finished(void);
void game_stop(void);
game_id_t game_active_id(void);
