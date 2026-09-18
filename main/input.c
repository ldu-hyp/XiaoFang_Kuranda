#include "input.h"

#include <math.h>
#include "esp_timer.h"
#include "imu.h"
#include "xf_config.h"

static xf_dir_t s_dir;
static int64_t s_dir_enter_us;
static int64_t s_last_repeat_us;
static int64_t s_last_shake_us;
static int64_t s_last_pause_us;
static imu_sample_t s_prev_sample;
static bool s_have_prev_sample;

esp_err_t input_init(void)
{
    s_dir = XF_DIR_NONE;
    s_dir_enter_us = 0;
    s_last_repeat_us = 0;
    s_last_shake_us = 0;
    s_last_pause_us = 0;
    s_have_prev_sample = false;
    return ESP_OK;
}

static xf_dir_t classify_direction(const imu_sample_t *s)
{
    const float ax = s->ax;
    const float ay = s->ay;

    if (fabsf(ax) < XF_TILT_RELEASE_G && fabsf(ay) < XF_TILT_RELEASE_G) {
        return XF_DIR_NONE;
    }

    /* Hysteresis: keep the previous direction inside the threshold band. */
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
    const int64_t now = esp_timer_get_time();
    const xf_dir_t previous = s_dir;
    const xf_dir_t next = classify_direction(&s);

    out->dir = next;
    out->dir_changed = next != previous;
    out->temperature_c = s.temperature_c;

    if (out->dir_changed) {
        if (previous != XF_DIR_NONE) {
            out->dir_released = true;
        }
        if (next != XF_DIR_NONE) {
            out->dir_pressed = true;
            s_dir_enter_us = now;
            s_last_repeat_us = now;
        } else {
            s_dir_enter_us = 0;
            s_last_repeat_us = 0;
        }
    } else if (next != XF_DIR_NONE &&
               now - s_dir_enter_us >= (int64_t)XF_DIR_REPEAT_DELAY_MS * 1000 &&
               now - s_last_repeat_us >= (int64_t)XF_DIR_REPEAT_INTERVAL_MS * 1000) {
        out->dir_repeat = true;
        s_last_repeat_us = now;
    }

    const float mag = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);

    if (fabsf(mag - 1.0f) > XF_SHAKE_THRESHOLD_G &&
        now - s_last_shake_us > 600000) {
        out->shake = true;
        s_last_shake_us = now;
    }

    if (s.az < XF_FLING_Z_G && s.az > -0.45f &&
        now - s_last_pause_us > 700000) {
        out->pause = true;
        s_last_pause_us = now;
    }

    out->face_down = s.az < XF_FACE_DOWN_G;

    bool physical_motion = false;
    if (s_have_prev_sample) {
        const float accel_delta =
            fabsf(s.ax - s_prev_sample.ax) +
            fabsf(s.ay - s_prev_sample.ay) +
            fabsf(s.az - s_prev_sample.az);

        const float max_gyro =
            fmaxf(fabsf(s.gx_dps),
                  fmaxf(fabsf(s.gy_dps), fabsf(s.gz_dps)));

        physical_motion =
            accel_delta >= XF_ACTIVITY_ACCEL_DELTA_G ||
            max_gyro >= XF_ACTIVITY_GYRO_DPS;
    }

    /*
     * A held non-neutral direction counts as active use. This prevents the
     * inactivity timer from sleeping while the user intentionally holds a tilt.
     */
    out->activity =
        next != XF_DIR_NONE ||
        out->dir_changed ||
        out->dir_repeat ||
        out->shake ||
        out->pause ||
        physical_motion;

    s_prev_sample = s;
    s_have_prev_sample = true;
    s_dir = next;
    return ESP_OK;
}
