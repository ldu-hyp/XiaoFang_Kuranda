#include "app/Application.hpp"
#include "esp_log.h"

extern "C" void app_main(void)
{
    static xiaofang::Application app;

    const esp_err_t err = app.init();
    if (err != ESP_OK) {
        ESP_LOGE("main", "app init failed: %s", esp_err_to_name(err));
        return;
    }

    app.run();
}
