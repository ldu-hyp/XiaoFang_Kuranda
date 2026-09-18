#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define XF_NET_MAX_PAYLOAD 32

typedef struct {
    uint8_t src[6];
    uint8_t len;
    uint8_t data[XF_NET_MAX_PAYLOAD];
} xf_net_packet_t;

esp_err_t network_init(void);
void network_shutdown(void);
bool network_is_ready(void);
void network_get_mac(uint8_t out_mac[6]);
esp_err_t network_send(const uint8_t dst[6], const void *data, size_t len);
esp_err_t network_broadcast(const void *data, size_t len);
bool network_poll(xf_net_packet_t *out);
