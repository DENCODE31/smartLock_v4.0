#pragma once
#include <app_network.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_APP_WIFI_PROV_COMPAT
#define APP_WIFI_EVENT APP_NETWORK_EVENT
typedef app_network_event_t app_wifi_event_t;
#define APP_WIFI_EVENT_QR_DISPLAY APP_NETWORK_EVENT_QR_DISPLAY
#define APP_WIFI_EVENT_PROV_TIMEOUT APP_NETWORK_EVENT_PROV_TIMEOUT
#define APP_WIFI_EVENT_PROV_RESTART APP_NETWORK_EVENT_PROV_RESTART
#define APP_WIFI_EVENT_PROV_CRED_MISMATCH APP_NETWORK_EVENT_PROV_CRED_MISMATCH
typedef app_network_pop_type_t app_wifi_pop_type_t;
#define app_wifi_init() app_network_init()
#define app_wifi_start(pop_type) app_network_start(pop_type)
#define app_wifi_set_custom_mfg_data(device_type, device_subtype) app_network_set_custom_mfg_data(device_type, device_subtype)
#define app_wifi_set_custom_pop(pop) app_network_set_custom_pop(pop)
#endif

#ifdef __cplusplus
}
#endif
