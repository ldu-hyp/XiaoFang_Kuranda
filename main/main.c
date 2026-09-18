#include "app.h"
#include "esp_log.h"

void app_main(void)
{
    esp_err_t err = app_init();
    if (err != ESP_OK) {
        ESP_LOGE("main", "app_init failed: %s", esp_err_to_name(err));
        return;
    }
    app_run();
}
