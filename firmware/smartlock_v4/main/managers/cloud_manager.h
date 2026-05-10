#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef void (*cloud_remote_cmd_cb_t)(bool unlock);

esp_err_t cloud_manager_init(cloud_remote_cmd_cb_t on_remote_cmd);
void cloud_manager_set_lock_state(bool locked);
void cloud_manager_notify_access(const char *user_id, const char *method);
