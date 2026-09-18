#include "buzzer.h"

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "xf_config.h"

typedef struct {
    uint32_t frequency_hz;
    uint32_t duration_ms;
} buzzer_note_t;

static bool s_enabled = true;
static bool s_ready;
static QueueHandle_t s_note_queue;

static void set_output(uint32_t frequency_hz, bool on)
{
    if (!s_ready && !s_note_queue) {
        return;
    }

    if (on && frequency_hz > 0) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_TIMER, frequency_hz);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL, 4096);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL, 0);
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, XF_BUZZER_LEDC_CHANNEL);
}

static void buzzer_task(void *arg)
{
    (void)arg;
    buzzer_note_t note;

    while (true) {
        if (xQueueReceive(s_note_queue, &note, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (!s_enabled || note.frequency_hz == 0) {
            continue;
        }

        set_output(note.frequency_hz, true);
        vTaskDelay(pdMS_TO_TICKS(note.duration_ms > 0 ? note.duration_ms : 50));
        set_output(0, false);
    }
}

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

    s_note_queue = xQueueCreate(12, sizeof(buzzer_note_t));
    if (!s_note_queue) {
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(
            buzzer_task,
            "xf_sound",
            2048,
            NULL,
            3,
            NULL) != pdPASS) {
        vQueueDelete(s_note_queue);
        s_note_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    s_ready = true;
    return ESP_OK;
}

void buzzer_set_enabled(bool enabled)
{
    s_enabled = enabled;
    if (!enabled) {
        buzzer_stop();
    }
}

bool buzzer_is_enabled(void)
{
    return s_enabled;
}

void buzzer_stop(void)
{
    if (s_note_queue) {
        xQueueReset(s_note_queue);
    }
    if (s_ready) {
        set_output(0, false);
    }
}

void buzzer_tone(uint32_t frequency_hz, uint32_t duration_ms)
{
    if (!s_enabled || !s_ready || !s_note_queue || frequency_hz == 0) {
        return;
    }

    const buzzer_note_t note = {
        .frequency_hz = frequency_hz,
        .duration_ms = duration_ms > 0 ? duration_ms : 50,
    };
    (void)xQueueSend(s_note_queue, &note, 0);
}

void buzzer_menu_move(void)
{
    buzzer_tone(1600, 35);
}

void buzzer_menu_enter(void)
{
    buzzer_tone(2300, 55);
}

void buzzer_score(void)
{
    buzzer_tone(2200, 45);
    buzzer_tone(3300, 55);
}

void buzzer_success(void)
{
    buzzer_tone(1800, 60);
    buzzer_tone(2500, 70);
    buzzer_tone(3400, 90);
}

void buzzer_game_over(void)
{
    buzzer_tone(800, 100);
    buzzer_tone(450, 140);
}

void buzzer_warning(void)
{
    buzzer_tone(900, 70);
    buzzer_tone(900, 70);
}
