#include "input.h"

#include <math.h>
#include "esp_timer.h"
#include "imu.h"
#include "xf_config.h"

static xf_dir_t s_dir;
static int64_t s_last_shake_us;
static int64_t s_last_pause_us;

esp_err_t input_init(void)
{
    s_dir = XF_DIR_NONE;
    s_last_shake_us = 0;
    s_last_pause_us = 0;
    return ESP_OK;
}

static xf_dir_t classify_direction(const imu_sample_t *s)
{
    float ax = s->ax;
    float ay = s->ay;
    if (fabsf(ax) < XF_TILT_RELEASE_G && fabsf(ay) < XF_TILT_RELEASE_G) {
        return XF_DIR_NONE;
    }
    if (fabsf(ax) < XF_TILT_THRESHOLD_G && fabsf(ay) < XF_TILT_THRESHOLD_G) {
        return s_dir;
    }
    if (fabsf(ax) >= fabsf(ay)) {
        return ax > 0 ? XF_DIR_LEFT : XF_DIR_RIGHT;
    }
    return ay > 0 ? XF_DIR_DOWN : XF_DIR_UP;
}

esp_err_t input_poll(xf_input_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    imu_sample_t s;
    ESP_RETURN_ON_ERROR(imu_read(&s), "input", "imu read");

    *out = (xf_input_t){0};
    xf_dir_t next = classify_direction(&s);
    out->dir = next;
    out->dir_changed = (next != s_dir);
    out->temperature_c = s.temperature_c;

    float mag = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
    int64_t now = esp_timer_get_time();

    if (fabsf(mag - 1.0f) > XF_SHAKE_THRESHOLD_G && now - s_last_shake_us > 600000) {
        out->shake = true;
        s_last_shake_us = now;
    }
    if (s.az < XF_FLING_Z_G && s.az > -0.45f && now - s_last_pause_us > 700000) {
        out->pause = true;
        s_last_pause_us = now;
    }
    out->face_down = s.az < XF_FACE_DOWN_G;
    out->activity = out->dir_changed || out->shake || out->pause;

    s_dir = next;
    return ESP_OK;
}
