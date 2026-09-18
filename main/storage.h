#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "xf_types.h"

typedef struct {
    uint8_t brightness;
    bool sound_enabled;
    uint16_t sleep_timeout_s;
} xf_settings_t;

esp_err_t storage_init(void);
esp_err_t storage_load_settings(xf_settings_t *out);
esp_err_t storage_save_settings(const xf_settings_t *settings);
uint16_t storage_get_high_score(xf_score_slot_t slot);
esp_err_t storage_set_high_score(xf_score_slot_t slot, uint16_t score);
