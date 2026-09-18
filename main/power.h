#pragma once

#include <stdbool.h>
#include <stdint.h>

void power_init(uint32_t timeout_ms);
void power_note_activity(void);
bool power_should_sleep(void);
void power_enter_deep_sleep(void) __attribute__((noreturn));
