#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "driver/i2c_types.h"
#include "esp_check.h"

/*
 * XiaoFang_Kuranda hardware allocation
 * Target: ESP32-WROOM-32D (classic ESP32, no native USB)
 *
 * Keep GPIO6..11 free for the module SPI flash and GPIO1/GPIO3 for UART0
 * programming/logging. Avoid boot-strapping pins for critical peripherals.
 */

#define XF_FW_VERSION                 "0.3.0"

#define XF_LED_WIDTH                  8
#define XF_LED_HEIGHT                 8
#define XF_LED_COUNT                  64
#define XF_PIN_WS2812                 GPIO_NUM_18
#define XF_LED_DEFAULT_BRIGHTNESS     40
#define XF_LED_MAX_MA                 800

#define XF_I2C_PORT                   I2C_NUM_0
#define XF_PIN_I2C_SDA                GPIO_NUM_21
#define XF_PIN_I2C_SCL                GPIO_NUM_22
#define XF_I2C_HZ                     400000
#define XF_MPU6050_ADDR               0x68
#define XF_PIN_MPU_INT                GPIO_NUM_33

/* Passive piezo element. Use a transistor/MOSFET for a low-ohm speaker. */
#define XF_PIN_BUZZER                 GPIO_NUM_25
#define XF_BUZZER_LEDC_TIMER          LEDC_TIMER_0
#define XF_BUZZER_LEDC_CHANNEL        LEDC_CHANNEL_0

/*
 * Future 4G modem: dedicated UART2 plus optional hardware flow control.
 * GPIO13/GPIO14 are used for RTS/CTS to avoid GPIO12/GPIO15 strap hazards.
 */
#define XF_MODEM_UART                 UART_NUM_2
#define XF_PIN_MODEM_RX               GPIO_NUM_16
#define XF_PIN_MODEM_TX               GPIO_NUM_17
#define XF_PIN_MODEM_RTS              GPIO_NUM_13
#define XF_PIN_MODEM_CTS              GPIO_NUM_14
#define XF_PIN_MODEM_DTR              GPIO_NUM_26
#define XF_PIN_MODEM_PWRKEY           GPIO_NUM_27
#define XF_PIN_MODEM_RI               GPIO_NUM_32

/* ADC1 inputs remain usable while Wi-Fi / ESP-NOW is active. */
#define XF_PIN_BATTERY_ADC            GPIO_NUM_34
#define XF_PIN_MIC_ADC                GPIO_NUM_35

#define XF_ESPNOW_CHANNEL             6
#define XF_ESPNOW_QUEUE_LEN           12

#define XF_APP_TICK_MS                50
#define XF_SLEEP_TIMEOUT_MS           30000

/* Direction auto-repeat: first step immediately, then repeat while held. */
#define XF_DIR_REPEAT_DELAY_MS        300
#define XF_DIR_REPEAT_INTERVAL_MS     130

/* Activity detection used by the inactivity sleep timer. */
#define XF_ACTIVITY_ACCEL_DELTA_G     0.04f
#define XF_ACTIVITY_GYRO_DPS          5.0f

/* Tune after final PCB/MPU6050 mounting orientation is fixed. */
#define XF_TILT_THRESHOLD_G           0.45f
#define XF_TILT_RELEASE_G             0.25f
#define XF_SHAKE_THRESHOLD_G          0.85f
#define XF_FACE_DOWN_G                (-0.75f)
#define XF_FLING_Z_G                  0.25f
