#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * Future 4G modem transport.
 * UART2 GPIO16/17 is dedicated to this layer.
 * Optional RTS/CTS uses GPIO14/15.
 * PWRKEY/DTR/RI remain model-specific and are intentionally not driven here.
 */
esp_err_t modem_uart_init(int baud, bool hardware_flow_control);
void modem_uart_deinit(void);
int modem_uart_write(const void *data, size_t len);
int modem_uart_read(void *data, size_t max_len, uint32_t timeout_ms);
