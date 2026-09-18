#pragma once

/*
 * Stable C boundary between ESP-IDF/hardware services and the C++ application
 * layer. Keeping this boundary explicit makes the upper layer replaceable
 * without coupling drivers to C++ runtime features.
 */
extern "C" {
#include "buzzer.h"
#include "display.h"
#include "imu.h"
#include "input.h"
#include "modem.h"
#include "network.h"
#include "power.h"
#include "storage.h"
#include "xf_types.h"
}

#include "xf_config.h"
