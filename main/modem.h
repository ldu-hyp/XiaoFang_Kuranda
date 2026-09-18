#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * Future 4G modem transport.
 * The firmware deliberately owns UART2 GPIO16/17 exclusively for this layer.
 * PWRKEY/DTR/RI are only reserved; their active levels depend on the modem.
 */
esp_err_t modem_uart_init(int baud);
void modem_uart_deinit(void);
int modem_uart_write(const void *data, size_t len);
int modem_uart_read(void *data, size_t max_len, uint32_t timeout_ms);
