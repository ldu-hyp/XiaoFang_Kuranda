#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "driver/i2c_types.h"
#include "esp_check.h"

/*
 * XiaoFang_Kuranda hardware allocation
 * Target: ESP32-S3-WROOM-1
 *
 * Design goals:
 * - Keep native USB D-/D+ on GPIO19/GPIO20.
 * - Avoid strapping pins GPIO0/GPIO3/GPIO45/GPIO46.
 * - Avoid GPIO26..37 so the same PCB plan remains friendly to S3 modules
 *   that internally use additional Flash/PSRAM signals.
 * - Keep the 4G modem on a dedicated UART with optional RTS/CTS.
 */

#define XF_FW_VERSION              "0.2.0"

#define XF_LED_WIDTH               8
#define XF_LED_HEIGHT              8
#define XF_LED_COUNT               64
#define XF_PIN_WS2812              GPIO_NUM_18
#define XF_LED_DEFAULT_BRIGHTNESS  40
#define XF_LED_MAX_MA              800

#define XF_I2C_PORT                I2C_NUM_0
#define XF_PIN_I2C_SDA             GPIO_NUM_8
#define XF_PIN_I2C_SCL             GPIO_NUM_9
#define XF_I2C_HZ                  400000
#define XF_MPU6050_ADDR            0x68
#define XF_PIN_MPU_INT             GPIO_NUM_7

/* Passive piezo. Use a transistor/MOSFET stage if the load is a low-ohm speaker. */
#define XF_PIN_BUZZER              GPIO_NUM_10
#define XF_BUZZER_LEDC_TIMER       LEDC_TIMER_0
#define XF_BUZZER_LEDC_CHANNEL     LEDC_CHANNEL_0

/*
 * Future 4G modem: dedicated UART2 plus optional hardware flow control.
 * PWRKEY/DTR/RI polarity and timing remain modem-model specific.
 */
#define XF_MODEM_UART              UART_NUM_2
#define XF_PIN_MODEM_RX            GPIO_NUM_16
#define XF_PIN_MODEM_TX            GPIO_NUM_17
#define XF_PIN_MODEM_DTR           GPIO_NUM_11
#define XF_PIN_MODEM_PWRKEY        GPIO_NUM_12
#define XF_PIN_MODEM_RI            GPIO_NUM_13
#define XF_PIN_MODEM_RTS           GPIO_NUM_14
#define XF_PIN_MODEM_CTS           GPIO_NUM_15

/* Native USB: GPIO19 = D-, GPIO20 = D+. Do not reuse them. */
#define XF_PIN_USB_DM              GPIO_NUM_19
#define XF_PIN_USB_DP              GPIO_NUM_20

/* ADC1 reservations remain usable while Wi-Fi / ESP-NOW is active. */
#define XF_PIN_BATTERY_ADC         GPIO_NUM_4
#define XF_PIN_MIC_ADC             GPIO_NUM_5

#define XF_ESPNOW_CHANNEL          6
#define XF_ESPNOW_QUEUE_LEN        12

#define XF_APP_TICK_MS             50
#define XF_SLEEP_TIMEOUT_MS        30000

/* Tune after final PCB/MPU6050 mounting orientation is fixed. */
#define XF_TILT_THRESHOLD_G        0.45f
#define XF_TILT_RELEASE_G          0.25f
#define XF_SHAKE_THRESHOLD_G       0.85f
#define XF_FACE_DOWN_G            (-0.75f)
#define XF_FLING_Z_G               0.25f
