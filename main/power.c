#include "power.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer.h"
#include "display.h"
#include "imu.h"
#include "network.h"
#include "xf_config.h"

static const char *TAG = "power";
static int64_t s_last_activity_us;
static int64_t s_next_sleep_attempt_us;
static uint32_t s_timeout_ms;

void power_init(uint32_t timeout_ms)
{
    s_timeout_ms = timeout_ms;
    s_last_activity_us = esp_timer_get_time();
    s_next_sleep_attempt_us = 0;

    gpio_set_direction(XF_PIN_MPU_INT, GPIO_MODE_INPUT);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        rtc_gpio_deinit(XF_PIN_MPU_INT);
        if (imu_restore_normal() != ESP_OK) {
            ESP_LOGW(TAG, "failed to fully restore MPU6050 after wake");
        }
    }
}

void power_note_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
}

bool power_should_sleep(void)
{
    if (!s_timeout_ms) {
        return false;
    }

    const int64_t idle_ms =
        (esp_timer_get_time() - s_last_activity_us) / 1000;
    return idle_ms >= s_timeout_ms;
}

static esp_err_t abort_sleep(esp_err_t err, const char *reason)
{
    ESP_LOGW(TAG, "deep sleep aborted: %s (%s)",
             reason, esp_err_to_name(err));
    (void)imu_restore_normal();

    const int64_t now = esp_timer_get_time();
    s_last_activity_us = now;
    s_next_sleep_attempt_us = now + 2000000;
    return err;
}

esp_err_t power_enter_deep_sleep(void)
{
    const int64_t now = esp_timer_get_time();
    if (now < s_next_sleep_attempt_us) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = imu_prepare_motion_wake();
    if (err != ESP_OK) {
        return abort_sleep(err, "MPU6050 motion interrupt setup failed");
    }

    /*
     * The MPU interrupt is configured as active-high + latched. Clear any
     * stale assertion before enabling EXT0, then verify the line is low.
     */
    err = imu_clear_interrupt();
    if (err != ESP_OK) {
        return abort_sleep(err, "failed to clear MPU6050 interrupt");
    }

    vTaskDelay(pdMS_TO_TICKS(15));
    if (gpio_get_level(XF_PIN_MPU_INT) != 0) {
        return abort_sleep(
            ESP_ERR_INVALID_STATE,
            "MPU6050 INT is already high; refusing immediate-wake loop"
        );
    }

    err = esp_sleep_enable_ext0_wakeup(XF_PIN_MPU_INT, 1);
    if (err != ESP_OK) {
        return abort_sleep(err, "EXT0 wake source setup failed");
    }

    ESP_LOGI(TAG,
             "entering deep sleep; MPU6050 motion interrupt on GPIO%d will wake",
             XF_PIN_MPU_INT);

    display_off();
    buzzer_stop();
    network_shutdown();

    esp_deep_sleep_start();
    __builtin_unreachable();
}
