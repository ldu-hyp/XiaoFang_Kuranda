#pragma once

#include "esp_err.h"
#include "xf_types.h"

esp_err_t input_init(void);
esp_err_t input_poll(xf_input_t *out);
