#include "imu.h"

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "xf_config.h"

static const char *TAG = "imu";
static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

enum {
    MPU_REG_SMPLRT_DIV      = 0x19,
    MPU_REG_CONFIG          = 0x1A,
    MPU_REG_GYRO_CONFIG     = 0x1B,
    MPU_REG_ACCEL_CONFIG    = 0x1C,
    MPU_REG_MOT_THR         = 0x1F,
    MPU_REG_MOT_DUR         = 0x20,
    MPU_REG_INT_PIN_CFG     = 0x37,
    MPU_REG_INT_ENABLE      = 0x38,
    MPU_REG_INT_STATUS      = 0x3A,
    MPU_REG_ACCEL_XOUT_H    = 0x3B,
    MPU_REG_MOT_DETECT_CTRL = 0x69,
    MPU_REG_PWR_MGMT_1      = 0x6B,
    MPU_REG_PWR_MGMT_2      = 0x6C,
    MPU_REG_WHO_AM_I        = 0x75,
};

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return i2c_master_transmit(s_dev, data, sizeof(data), 100);
}

static esp_err_t read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, data, len, 100);
}

esp_err_t imu_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = XF_I2C_PORT,
        .sda_io_num = XF_PIN_I2C_SDA,
        .scl_io_num = XF_PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "new I2C bus failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XF_MPU6050_ADDR,
        .scl_speed_hz = XF_I2C_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG, "add MPU6050 failed");

    uint8_t who = 0;
    ESP_RETURN_ON_ERROR(read_regs(MPU_REG_WHO_AM_I, &who, 1), TAG, "WHO_AM_I failed");
    if ((who & 0x7E) != 0x68) {
        ESP_LOGE(TAG, "unexpected WHO_AM_I=0x%02x", who);
        return ESP_ERR_NOT_FOUND;
    }

    /* PLL on X gyro, 1 kHz sample base, DLPF ~44 Hz, +/-2g, +/-250 dps. */
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_PWR_MGMT_1, 0x01), TAG, "wake failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_PWR_MGMT_2, 0x00), TAG, "axes enable failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_SMPLRT_DIV, 9), TAG, "sample divider failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_CONFIG, 0x03), TAG, "DLPF failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_GYRO_CONFIG, 0x00), TAG, "gyro range failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_ACCEL_CONFIG, 0x00), TAG, "accel range failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_ENABLE, 0x00), TAG, "interrupt disable failed");
    return imu_clear_interrupt();
}

esp_err_t imu_read(imu_sample_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t b[14];
    ESP_RETURN_ON_ERROR(read_regs(MPU_REG_ACCEL_XOUT_H, b, sizeof(b)), TAG, "sample read failed");

    int16_t ax = (int16_t)((b[0] << 8) | b[1]);
    int16_t ay = (int16_t)((b[2] << 8) | b[3]);
    int16_t az = (int16_t)((b[4] << 8) | b[5]);
    int16_t t  = (int16_t)((b[6] << 8) | b[7]);
    int16_t gx = (int16_t)((b[8] << 8) | b[9]);
    int16_t gy = (int16_t)((b[10] << 8) | b[11]);
    int16_t gz = (int16_t)((b[12] << 8) | b[13]);

    out->ax = ax / 16384.0f;
    out->ay = ay / 16384.0f;
    out->az = az / 16384.0f;
    out->temperature_c = t / 340.0f + 36.53f;
    out->gx_dps = gx / 131.0f;
    out->gy_dps = gy / 131.0f;
    out->gz_dps = gz / 131.0f;
    return ESP_OK;
}

esp_err_t imu_clear_interrupt(void)
{
    uint8_t status;
    return read_regs(MPU_REG_INT_STATUS, &status, 1);
}

esp_err_t imu_prepare_motion_wake(void)
{
    /*
     * Motion interrupt, active-high and latched until INT_STATUS is read.
     * Threshold/duration are deliberately conservative and should be tuned
     * after the final mechanical assembly is available.
     */
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_ENABLE, 0x00), TAG, "irq off failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_MOT_THR, 12), TAG, "motion threshold failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_MOT_DUR, 20), TAG, "motion duration failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_MOT_DETECT_CTRL, 0x15), TAG, "motion control failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_PIN_CFG, 0x20), TAG, "int pin failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_ENABLE, 0x40), TAG, "motion irq failed");
    return ESP_OK;
}

esp_err_t imu_restore_normal(void)
{
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_ENABLE, 0x00), TAG, "irq off failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_INT_PIN_CFG, 0x00), TAG, "int pin normal failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_PWR_MGMT_1, 0x01), TAG, "normal mode failed");
    ESP_RETURN_ON_ERROR(write_reg(MPU_REG_PWR_MGMT_2, 0x00), TAG, "axes restore failed");
    return imu_clear_interrupt();
}
