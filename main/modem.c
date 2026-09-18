#include "modem.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "xf_config.h"

static bool s_ready;

esp_err_t modem_uart_init(int baud, bool hardware_flow_control)
{
    if (s_ready) {
        return ESP_OK;
    }

    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = hardware_flow_control ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = hardware_flow_control ? 122 : 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(uart_param_config(XF_MODEM_UART, &cfg), "modem", "config");

    const int rts = hardware_flow_control ? XF_PIN_MODEM_RTS : UART_PIN_NO_CHANGE;
    const int cts = hardware_flow_control ? XF_PIN_MODEM_CTS : UART_PIN_NO_CHANGE;
    ESP_RETURN_ON_ERROR(
        uart_set_pin(XF_MODEM_UART, XF_PIN_MODEM_TX, XF_PIN_MODEM_RX, rts, cts),
        "modem", "pins"
    );

    ESP_RETURN_ON_ERROR(
        uart_driver_install(XF_MODEM_UART, 4096, 4096, 0, NULL, 0),
        "modem", "driver"
    );

    s_ready = true;
    return ESP_OK;
}

void modem_uart_deinit(void)
{
    if (s_ready) {
        uart_driver_delete(XF_MODEM_UART);
    }
    s_ready = false;
}

int modem_uart_write(const void *data, size_t len)
{
    if (!s_ready) {
        return -1;
    }
    return uart_write_bytes(XF_MODEM_UART, data, len);
}

int modem_uart_read(void *data, size_t max_len, uint32_t timeout_ms)
{
    if (!s_ready) {
        return -1;
    }
    return uart_read_bytes(XF_MODEM_UART, data, max_len, pdMS_TO_TICKS(timeout_ms));
}
