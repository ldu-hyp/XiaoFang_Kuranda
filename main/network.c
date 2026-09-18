#include "network.h"

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "xf_config.h"

static const char *TAG = "network";
static QueueHandle_t s_rx_queue;
static bool s_ready;
static bool s_wifi_inited;
static bool s_wifi_started;
static bool s_espnow_inited;
static bool s_recv_registered;
static uint8_t s_mac[6];
static const uint8_t BROADCAST[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!s_rx_queue || !info || !data || len <= 0) {
        return;
    }

    xf_net_packet_t packet = {0};
    memcpy(packet.src, info->src_addr, 6);
    packet.len =
        len > XF_NET_MAX_PAYLOAD ? XF_NET_MAX_PAYLOAD : (uint8_t)len;
    memcpy(packet.data, data, packet.len);

    /* Never block the Wi-Fi callback. Dropping a packet is safer than stalling it. */
    (void)xQueueSend(s_rx_queue, &packet, 0);
}

static esp_err_t ensure_peer(const uint8_t mac[6])
{
    if (esp_now_is_peer_exist(mac)) {
        return ESP_OK;
    }

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = XF_ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer);
}

static void cleanup(void)
{
    s_ready = false;

    if (s_recv_registered) {
        (void)esp_now_unregister_recv_cb();
        s_recv_registered = false;
    }

    if (s_espnow_inited) {
        (void)esp_now_deinit();
        s_espnow_inited = false;
    }

    if (s_rx_queue) {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
    }

    if (s_wifi_started) {
        (void)esp_wifi_stop();
        s_wifi_started = false;
    }

    if (s_wifi_inited) {
        (void)esp_wifi_deinit();
        s_wifi_inited = false;
    }
}

esp_err_t network_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    cleanup();

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_inited = true;

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_started = true;

    err = esp_wifi_set_channel(XF_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        goto fail;
    }

    s_rx_queue = xQueueCreate(XF_ESPNOW_QUEUE_LEN, sizeof(xf_net_packet_t));
    if (!s_rx_queue) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }

    err = esp_now_init();
    if (err != ESP_OK) {
        goto fail;
    }
    s_espnow_inited = true;

    err = esp_now_register_recv_cb(recv_cb);
    if (err != ESP_OK) {
        goto fail;
    }
    s_recv_registered = true;

    err = esp_wifi_get_mac(WIFI_IF_STA, s_mac);
    if (err != ESP_OK) {
        goto fail;
    }

    err = ensure_peer(BROADCAST);
    if (err != ESP_OK) {
        goto fail;
    }

    s_ready = true;
    ESP_LOGI(TAG,
             "ESP-NOW ready ch=%d MAC=%02x:%02x:%02x:%02x:%02x:%02x",
             XF_ESPNOW_CHANNEL,
             s_mac[0], s_mac[1], s_mac[2],
             s_mac[3], s_mac[4], s_mac[5]);
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "network initialization failed: %s", esp_err_to_name(err));
    cleanup();
    return err;
}

void network_shutdown(void)
{
    cleanup();
}

bool network_is_ready(void)
{
    return s_ready;
}

void network_get_mac(uint8_t out_mac[6])
{
    if (out_mac) {
        memcpy(out_mac, s_mac, 6);
    }
}

esp_err_t network_send(const uint8_t dst[6], const void *data, size_t len)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!dst || !data || len == 0 || len > XF_NET_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(ensure_peer(dst), TAG, "peer add");
    return esp_now_send(dst, data, len);
}

esp_err_t network_broadcast(const void *data, size_t len)
{
    return network_send(BROADCAST, data, len);
}

bool network_poll(xf_net_packet_t *out)
{
    return s_rx_queue &&
           out &&
           xQueueReceive(s_rx_queue, out, 0) == pdTRUE;
}
