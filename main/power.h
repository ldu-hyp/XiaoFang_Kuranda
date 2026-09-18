#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

void power_init(uint32_t timeout_ms);
void power_note_activity(void);
bool power_should_sleep(void);

/*
 * Enters deep sleep on success and therefore does not return.
 * If the MPU6050 wake source cannot be prepared safely, returns an error
 * instead of putting the device into a state with no reliable wake source.
 */
esp_err_t power_enter_deep_sleep(void);
