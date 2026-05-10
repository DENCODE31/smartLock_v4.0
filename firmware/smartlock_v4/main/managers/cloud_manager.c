#include "cloud_manager.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_standard_devices.h"
#include "esp_rmaker_ota.h"
#include "esp_rmaker_schedule.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "CLOUD_MGR";

#define WIFI_CONNECTED_BIT  BIT0

static EventGroupHandle_t   s_wifi_events;
static esp_rmaker_device_t *s_lock_device;
static esp_rmaker_param_t  *s_power_param;
static esp_rmaker_param_t  *s_last_access_param;
static esp_rmaker_param_t  *s_access_method_param;
static cloud_remote_cmd_cb_t s_remote_cb;

// ─── WiFi + Provisioning ──────────────────────────────────────────────────────

static void event_handler(void *arg, esp_event_base_t base,
                           int32_t id, void *data)
{
    if (base == WIFI_PROV_EVENT) {
        switch (id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning iniciado — abre la app RainMaker");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *cfg = (wifi_sta_config_t *)data;
                ESP_LOGI(TAG, "Credenciales recibidas: SSID=%s", cfg->ssid);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = data;
                ESP_LOGE(TAG, "Fallo provisioning: %s",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "auth error" : "AP no encontrado");
                wifi_prov_mgr_reset_sm_state_on_failure();
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Credenciales WiFi guardadas");
                break;
            case WIFI_PROV_END:
                wifi_prov_mgr_deinit();
                break;
        }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi desconectado — reintentando...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_and_provision(void)
{
    s_wifi_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,      ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,        IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_prov_mgr_config_t prov_cfg = {
        .scheme = wifi_prov_scheme_ble,
#ifdef HELTEC_V3
        // ESP32-S3: solo BLE, sin classic BT
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
#else
        // ESP32-WROOM-32E: libera classic BT + BLE al terminar
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
#endif
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        char svc_name[32];
        snprintf(svc_name, sizeof(svc_name), "PROV_%s", RAINMAKER_DEVICE_NAME);

        ESP_LOGI(TAG, "Sin credenciales — BLE provisioning activo");
        ESP_LOGI(TAG, "Nombre BLE: %s  |  POP: %s", svc_name, RAINMAKER_PROV_POP);
        ESP_LOGI(TAG, "Abre la app RainMaker > Add Device > escanea el QR o ingresa POP");

        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
            WIFI_PROV_SECURITY_1,
            RAINMAKER_PROV_POP,
            svc_name,
            NULL
        ));
    } else {
        ESP_LOGI(TAG, "WiFi ya provisionado — conectando...");
        wifi_prov_mgr_deinit();
        esp_wifi_connect();
    }

    xEventGroupWaitBits(s_wifi_events, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    return ESP_OK;
}

// ─── RainMaker ────────────────────────────────────────────────────────────────

static esp_err_t lock_write_cb(const esp_rmaker_device_t *device,
                                const esp_rmaker_param_t  *param,
                                const esp_rmaker_param_val_t val,
                                void *priv_data,
                                esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool unlock = val.val.b;
        ESP_LOGI(TAG, "Comando remoto (Alexa/App): %s", unlock ? "UNLOCK" : "LOCK");
        if (s_remote_cb) {
            s_remote_cb(unlock);
        }
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

static esp_err_t rainmaker_init(void)
{
    esp_rmaker_config_t cfg = { .enable_time_sync = true };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&cfg, RAINMAKER_NODE_NAME, RAINMAKER_NODE_TYPE);
    if (!node) {
        ESP_LOGE(TAG, "Fallo al inicializar nodo RainMaker");
        return ESP_FAIL;
    }

    // Switch device: Alexa lo reconoce como enchufe/interruptor inteligente
    // ON = puerta desbloqueada | OFF = puerta bloqueada
    s_lock_device = esp_rmaker_switch_device_create(RAINMAKER_DEVICE_NAME, NULL, false);
    if (!s_lock_device) {
        ESP_LOGE(TAG, "Fallo al crear dispositivo lock");
        return ESP_FAIL;
    }

    s_power_param = esp_rmaker_device_get_param_by_type(s_lock_device, ESP_RMAKER_PARAM_POWER);

    s_last_access_param = esp_rmaker_param_create(
        "Last Access", NULL,
        esp_rmaker_str("None"),
        PROP_FLAG_READ
    );
    esp_rmaker_device_add_param(s_lock_device, s_last_access_param);

    s_access_method_param = esp_rmaker_param_create(
        "Access Method", NULL,
        esp_rmaker_str("None"),
        PROP_FLAG_READ
    );
    esp_rmaker_device_add_param(s_lock_device, s_access_method_param);

    esp_rmaker_device_add_cb(s_lock_device, lock_write_cb, NULL);
    esp_rmaker_node_add_device(node, s_lock_device);

    esp_rmaker_ota_enable_default();
    esp_rmaker_schedule_enable();

    ESP_ERROR_CHECK(esp_rmaker_start());

    ESP_LOGI(TAG, "RainMaker iniciado — habilita el skill 'Espressif RainMaker' en Alexa");
    return ESP_OK;
}

// ─── API pública ──────────────────────────────────────────────────────────────

esp_err_t cloud_manager_init(cloud_remote_cmd_cb_t on_remote_cmd)
{
    s_remote_cb = on_remote_cmd;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(wifi_init_and_provision());
    ESP_ERROR_CHECK(rainmaker_init());

    ESP_LOGI(TAG, "Cloud manager listo");
    return ESP_OK;
}

void cloud_manager_set_lock_state(bool locked)
{
    if (!s_power_param) return;
    // locked=true → power OFF (false) | unlocked=true → power ON (true)
    esp_rmaker_param_update_and_report(s_power_param, esp_rmaker_bool(!locked));
}

void cloud_manager_notify_access(const char *user_id, const char *method)
{
    if (s_last_access_param) {
        esp_rmaker_param_update_and_report(s_last_access_param, esp_rmaker_str(user_id));
    }
    if (s_access_method_param) {
        esp_rmaker_param_update_and_report(s_access_method_param, esp_rmaker_str(method));
    }
}
