#include "storage.h"

#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "xf_config.h"

static nvs_handle_t s_nvs;
static const char *score_keys[XF_SCORE_COUNT] = {
    "hi_snake", "hi_cube", "hi_dodge", "hi_pong"
};

esp_err_t storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, "storage", "nvs init");
    return nvs_open("xiaofang", NVS_READWRITE, &s_nvs);
}

esp_err_t storage_load_settings(xf_settings_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (xf_settings_t){
        .brightness = XF_LED_DEFAULT_BRIGHTNESS,
        .sound_enabled = true,
        .sleep_timeout_s = XF_SLEEP_TIMEOUT_MS / 1000,
    };

    uint8_t b;
    uint16_t s;
    if (nvs_get_u8(s_nvs, "brightness", &b) == ESP_OK) out->brightness = b;
    if (nvs_get_u8(s_nvs, "sound", &b) == ESP_OK) out->sound_enabled = b != 0;
    if (nvs_get_u16(s_nvs, "sleep_s", &s) == ESP_OK) out->sleep_timeout_s = s;
    return ESP_OK;
}

esp_err_t storage_save_settings(const xf_settings_t *settings)
{
    if (!settings) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(nvs_set_u8(s_nvs, "brightness", settings->brightness), "storage", "brightness");
    ESP_RETURN_ON_ERROR(nvs_set_u8(s_nvs, "sound", settings->sound_enabled ? 1 : 0), "storage", "sound");
    ESP_RETURN_ON_ERROR(nvs_set_u16(s_nvs, "sleep_s", settings->sleep_timeout_s), "storage", "sleep");
    return nvs_commit(s_nvs);
}

uint16_t storage_get_high_score(xf_score_slot_t slot)
{
    if ((unsigned)slot >= XF_SCORE_COUNT) return 0;
    uint16_t value = 0;
    nvs_get_u16(s_nvs, score_keys[slot], &value);
    return value;
}

esp_err_t storage_set_high_score(xf_score_slot_t slot, uint16_t score)
{
    if ((unsigned)slot >= XF_SCORE_COUNT) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(nvs_set_u16(s_nvs, score_keys[slot], score), "storage", "score");
    return nvs_commit(s_nvs);
}
