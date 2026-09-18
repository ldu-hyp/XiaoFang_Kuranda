#include "buzzer.h"

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xf_config.h"

static bool s_enabled = true;
static bool s_ready;

esp_err_t buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = XF_BUZZER_LEDC_TIMER,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), "buzzer", "timer");

    ledc_channel_config_t channel = {
        .gpio_num = XF_PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = XF_BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = XF_BUZZER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), "buzzer", "channel");
    s_ready = true;
    return ESP_OK;
}

void buzzer_set_enabled(bool enabled) { s_enabled = enabled; }
bool buzzer_is_enabled(void) { return s_enabled; }

void buzzer_stop(void)
{
    if (!s_ready) return;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL);
}

void buzzer_tone(uint32_t frequency_hz, uint32_t duration_ms)
{
    if (!s_enabled || !s_ready || frequency_hz == 0) return;
    ledc_set_freq(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_TIMER, frequency_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL, 4096);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL);
    if (duration_ms) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        buzzer_stop();
    }
}

void buzzer_menu_move(void) { buzzer_tone(1600, 35); }
void buzzer_menu_enter(void) { buzzer_tone(2300, 55); }
void buzzer_score(void)
{
    buzzer_tone(2200, 45);
    buzzer_tone(3300, 55);
}
void buzzer_game_over(void)
{
    buzzer_tone(800, 100);
    buzzer_tone(450, 140);
}
