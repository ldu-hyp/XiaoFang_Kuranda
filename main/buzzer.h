#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t buzzer_init(void);
void buzzer_set_enabled(bool enabled);
bool buzzer_is_enabled(void);
void buzzer_stop(void);

/* Non-blocking: notes are queued to a dedicated sound task. */
void buzzer_tone(uint32_t frequency_hz, uint32_t duration_ms);
void buzzer_menu_move(void);
void buzzer_menu_enter(void);
void buzzer_score(void);
void buzzer_success(void);
void buzzer_game_over(void);
void buzzer_warning(void);
