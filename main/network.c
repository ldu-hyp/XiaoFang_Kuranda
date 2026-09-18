#include "network.h"

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "xf_config.h"

static const char *TAG = "network";
static QueueHandle_t s_rx_queue;
static bool s_ready;
static uint8_t s_mac[6];
static const uint8_t BROADCAST[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!s_rx_queue || !info || !data || len <= 0) return;
    xf_net_packet_t p = {0};
    memcpy(p.src, info->src_addr, 6);
    p.len = len > XF_NET_MAX_PAYLOAD ? XF_NET_MAX_PAYLOAD : (uint8_t)len;
    memcpy(p.data, data, p.len);
    xQueueSend(s_rx_queue, &p, 0);
}

static esp_err_t ensure_peer(const uint8_t mac[6])
{
    if (esp_now_is_peer_exist(mac)) return ESP_OK;
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = XF_ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer);
}

esp_err_t network_init(void)
{
    if (s_ready) return ESP_OK;

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "wifi storage");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "wifi mode");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start");
    ESP_RETURN_ON_ERROR(esp_wifi_set_channel(XF_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE), TAG, "wifi channel");

    ESP_RETURN_ON_ERROR(esp_now_init(), TAG, "esp-now init");
    ESP_RETURN_ON_ERROR(esp_now_register_recv_cb(recv_cb), TAG, "recv cb");

    esp_wifi_get_mac(WIFI_IF_STA, s_mac);
    s_rx_queue = xQueueCreate(XF_ESPNOW_QUEUE_LEN, sizeof(xf_net_packet_t));
    if (!s_rx_queue) return ESP_ERR_NO_MEM;
    ESP_RETURN_ON_ERROR(ensure_peer(BROADCAST), TAG, "broadcast peer");

    s_ready = true;
    ESP_LOGI(TAG, "ESP-NOW ready on channel %d, MAC %02x:%02x:%02x:%02x:%02x:%02x",
             XF_ESPNOW_CHANNEL, s_mac[0],s_mac[1],s_mac[2],s_mac[3],s_mac[4],s_mac[5]);
    return ESP_OK;
}

void network_shutdown(void)
{
    if (!s_ready) return;
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    if (s_rx_queue) {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
    }
    s_ready = false;
}

bool network_is_ready(void) { return s_ready; }

void network_get_mac(uint8_t out_mac[6])
{
    if (out_mac) memcpy(out_mac, s_mac, 6);
}

esp_err_t network_send(const uint8_t dst[6], const void *data, size_t len)
{
    if (!s_ready || !dst || !data || !len || len > XF_NET_MAX_PAYLOAD) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ensure_peer(dst), TAG, "peer add");
    return esp_now_send(dst, data, len);
}

esp_err_t network_broadcast(const void *data, size_t len)
{
    return network_send(BROADCAST, data, len);
}

bool network_poll(xf_net_packet_t *out)
{
    return s_rx_queue && out && xQueueReceive(s_rx_queue, out, 0) == pdTRUE;
}
