#include "power.h"

#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "buzzer.h"
#include "display.h"
#include "imu.h"
#include "network.h"
#include "xf_config.h"

static int64_t s_last_activity_us;
static uint32_t s_timeout_ms;

void power_init(uint32_t timeout_ms)
{
    s_timeout_ms = timeout_ms;
    s_last_activity_us = esp_timer_get_time();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        rtc_gpio_deinit(XF_PIN_MPU_INT);
        imu_clear_interrupt();
        imu_restore_normal();
    }
}

void power_note_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
}

bool power_should_sleep(void)
{
    if (!s_timeout_ms) return false;
    int64_t idle_ms = (esp_timer_get_time() - s_last_activity_us) / 1000;
    return idle_ms >= s_timeout_ms;
}

void power_enter_deep_sleep(void)
{
    ESP_LOGI("power", "entering deep sleep; MPU6050 motion interrupt will wake GPIO%d", XF_PIN_MPU_INT);
    display_off();
    buzzer_stop();
    network_shutdown();
    imu_prepare_motion_wake();

    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(XF_PIN_MPU_INT, 1));
    esp_deep_sleep_start();
    __builtin_unreachable();
}
