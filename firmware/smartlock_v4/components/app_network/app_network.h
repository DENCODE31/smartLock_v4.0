#pragma once
#include <esp_err.h>
#include <esp_event.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
    uint8_t header[2];
    uint8_t app_id[3];
    uint8_t version;
    uint8_t customer_id[2];
} app_network_mfg_data_prefix_t;

#define MFG_DATA_HEADER                     0xe5, 0x02
#define MFG_DATA_APP_ID                     'N', 'o', 'v'
#define MFG_DATA_VERSION                    'a'
#define MFG_DATA_CUSTOMER_ID                0x00, 0x01

#define MFG_DATA_DEVICE_TYPE_LIGHT              0x0005
#define MFG_DATA_DEVICE_TYPE_SWITCH             0x0080
#define MFG_DATA_DEVICE_TYPE_USER_AUTH          0x0101

#define MFG_DATA_DEVICE_SUBTYPE_SWITCH              0x01
#define MFG_DATA_DEVICE_SUBTYPE_LIGHT               0x01

#define MFG_DATA_DEVICE_EXTRA_CODE          0x00

ESP_EVENT_DECLARE_BASE(APP_NETWORK_EVENT);

typedef enum {
    APP_NETWORK_EVENT_QR_DISPLAY = 1,
    APP_NETWORK_EVENT_PROV_TIMEOUT,
    APP_NETWORK_EVENT_PROV_RESTART,
    APP_NETWORK_EVENT_PROV_CRED_MISMATCH,
} app_network_event_t;

typedef enum {
    POP_TYPE_MAC,
    POP_TYPE_RANDOM,
    POP_TYPE_NONE,
    POP_TYPE_CUSTOM
} app_network_pop_type_t;

void app_network_init(void);
esp_err_t app_network_start(app_network_pop_type_t pop_type);
esp_err_t app_network_set_custom_mfg_data_prefix(const app_network_mfg_data_prefix_t *mfg_data_prefix);
esp_err_t app_network_set_custom_mfg_data(uint16_t device_type, uint8_t device_subtype);
esp_err_t app_network_set_custom_pop(const char *pop);
char *app_network_get_device_pop(app_network_pop_type_t pop_type);
char *app_network_get_device_pop_default(void);
char *app_network_get_device_service_name(void);

#ifdef __cplusplus
}
#endif
