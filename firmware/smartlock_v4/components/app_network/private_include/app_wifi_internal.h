#pragma once
#include <esp_err.h>
#include <esp_event.h>
#include <esp_idf_version.h>
#include <app_network.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_wifi_internal_init(void);
esp_err_t app_wifi_internal_start(const char *pop, const char *service_name,
                                  const char *service_key, uint8_t *mfg_data,
                                  size_t mfg_data_len, bool *provisioned);

#ifdef __cplusplus
}
#endif
