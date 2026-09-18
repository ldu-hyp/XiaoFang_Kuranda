#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "driver/i2c_types.h"
#include "esp_check.h"

/*
 * XiaoFang_Kuranda hardware allocation
 * Target: ESP32-WROOM-32D (classic ESP32)
 *
 * Avoid GPIO6-11 (SPI flash), UART0 GPIO1/3 (console/programming),
 * and boot-strapping pins for normal peripherals where practical.
 */

#define XF_FW_VERSION              "0.1.0"

#define XF_LED_WIDTH               8
#define XF_LED_HEIGHT              8
#define XF_LED_COUNT               64
#define XF_PIN_WS2812              GPIO_NUM_18
#define XF_LED_DEFAULT_BRIGHTNESS  40
#define XF_LED_MAX_MA              800

#define XF_I2C_PORT                I2C_NUM_0
#define XF_PIN_I2C_SDA             GPIO_NUM_21
#define XF_PIN_I2C_SCL             GPIO_NUM_22
#define XF_I2C_HZ                  400000
#define XF_MPU6050_ADDR            0x68
#define XF_PIN_MPU_INT             GPIO_NUM_33

/* Passive piezo element / passive buzzer. Use a transistor for a larger speaker. */
#define XF_PIN_BUZZER              GPIO_NUM_25
#define XF_BUZZER_LEDC_TIMER       LEDC_TIMER_0
#define XF_BUZZER_LEDC_CHANNEL     LEDC_CHANNEL_0

/*
 * Future 4G modem allocation. UART2 is intentionally kept free from other
 * subsystems so a modem can be added without changing the PCB pin map.
 */
#define XF_MODEM_UART              UART_NUM_2
#define XF_PIN_MODEM_RX            GPIO_NUM_16
#define XF_PIN_MODEM_TX            GPIO_NUM_17
#define XF_PIN_MODEM_DTR           GPIO_NUM_26
#define XF_PIN_MODEM_PWRKEY        GPIO_NUM_27
#define XF_PIN_MODEM_RI            GPIO_NUM_32

/* Reserved analog inputs (ADC1, compatible with Wi-Fi/ESP-NOW use). */
#define XF_PIN_BATTERY_ADC         GPIO_NUM_34
#define XF_PIN_MIC_ADC             GPIO_NUM_35

#define XF_ESPNOW_CHANNEL          6
#define XF_ESPNOW_QUEUE_LEN        12

#define XF_APP_TICK_MS             50
#define XF_MENU_REPEAT_MS          350
#define XF_SLEEP_TIMEOUT_MS        30000

/* MPU6050 / gesture thresholds, tune after the final PCB orientation is known. */
#define XF_TILT_THRESHOLD_G        0.45f
#define XF_TILT_RELEASE_G          0.25f
#define XF_SHAKE_THRESHOLD_G       0.85f
#define XF_FACE_DOWN_G            (-0.75f)
#define XF_FLING_Z_G               0.25f
