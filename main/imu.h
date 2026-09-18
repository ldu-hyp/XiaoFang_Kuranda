#pragma once

#include "esp_err.h"

typedef struct {
    float ax;
    float ay;
    float az;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float temperature_c;
} imu_sample_t;

esp_err_t imu_init(void);
esp_err_t imu_read(imu_sample_t *out);
esp_err_t imu_prepare_motion_wake(void);
esp_err_t imu_restore_normal(void);
esp_err_t imu_clear_interrupt(void);
