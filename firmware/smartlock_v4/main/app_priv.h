#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_LOCK_STATE   true

extern esp_rmaker_device_t *lock_device;

void app_driver_init(void);
int app_driver_set_state(bool locked);
bool app_driver_get_state(void);
