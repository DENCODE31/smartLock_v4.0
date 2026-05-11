#include <sdkconfig.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_idf_version.h>

#include <app_network.h>
#include <app_wifi_internal.h>
#include <esp_netif_types.h>

#include <esp_mac.h>

#include <network_provisioning/manager.h>
#ifdef CONFIG_APP_NETWORK_PROV_TRANSPORT_BLE
#include <network_provisioning/scheme_ble.h>
#else
#include <network_provisioning/scheme_softap.h>
#endif

#ifdef CONFIG_APP_NETWORK_PROV_SHOW_QR
#include <qrcode.h>
#endif

#include <nvs.h>
#include <nvs_flash.h>
#include <esp_timer.h>

ESP_EVENT_DEFINE_BASE(APP_NETWORK_EVENT);
static const char *TAG = "app_network";

#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
static const int NETWORK_CONNECTED_EVENT = BIT0;
static const int NETWORK_PROV_ENDED_EVENT = BIT1;
static const int NETWORK_PROV_STARTED_EVENT = BIT2;
static EventGroupHandle_t network_event_group;
#endif

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://rainmaker.espressif.com/qrcode.html"

#define RANDOM_NVS_PARTITION_NAME  CONFIG_APP_NETWORK_FACTORY_NVS_PARTITION_NAME
#define RANDOM_NVS_NAMESPACE       CONFIG_APP_NETWORK_PROV_DATA_NAMESPACE
#define RANDOM_NVS_KEY             CONFIG_APP_NETWORK_PROV_RANDOM_NVS_KEY

#define POP_STR_SIZE    9
static esp_timer_handle_t prov_stop_timer;
#define APP_NETWORK_PROV_TIMEOUT_PERIOD   CONFIG_APP_NETWORK_PROV_TIMEOUT_PERIOD
static uint64_t prov_timeout_period = (APP_NETWORK_PROV_TIMEOUT_PERIOD * 60 * 1000000LL);

static app_network_mfg_data_prefix_t g_mfg_data_prefix = {
    .header = {MFG_DATA_HEADER},
    .app_id = {MFG_DATA_APP_ID},
    .version = MFG_DATA_VERSION,
    .customer_id = {MFG_DATA_CUSTOMER_ID},
};
static uint8_t *custom_mfg_data = NULL;
static size_t custom_mfg_data_len = 0;

esp_err_t app_network_set_custom_mfg_data_prefix(const app_network_mfg_data_prefix_t *mfg_data_prefix)
{
    if (!mfg_data_prefix) return ESP_ERR_INVALID_ARG;
    memcpy(&g_mfg_data_prefix, mfg_data_prefix, sizeof(app_network_mfg_data_prefix_t));
    return ESP_OK;
}

esp_err_t app_network_set_custom_mfg_data(uint16_t device_type, uint8_t device_subtype)
{
    size_t mfg_data_len = sizeof(g_mfg_data_prefix) + 4;
    custom_mfg_data = (uint8_t *)malloc(mfg_data_len);
    if (!custom_mfg_data) return ESP_ERR_NO_MEM;

    memcpy(custom_mfg_data, &g_mfg_data_prefix, sizeof(g_mfg_data_prefix));
    custom_mfg_data[8] = 0xff & (device_type >> 8);
    custom_mfg_data[9] = 0xff & device_type;
    custom_mfg_data[10] = device_subtype;
    custom_mfg_data[11] = MFG_DATA_DEVICE_EXTRA_CODE;
    custom_mfg_data_len = mfg_data_len;
    return ESP_OK;
}

static void app_network_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !transport) return;
    char payload[150];
    if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                PROV_QR_VERSION, name, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                ",\"transport\":\"%s\"}",
                PROV_QR_VERSION, name, transport);
    }
#ifdef CONFIG_APP_NETWORK_PROV_SHOW_QR
    ESP_LOGI(TAG, "Scan this QR code from the ESP RainMaker phone app for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    cfg.max_qrcode_version = 5;
    esp_qrcode_generate(&cfg, payload);
#endif
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
    esp_event_post(APP_NETWORK_EVENT, APP_NETWORK_EVENT_QR_DISPLAY, payload, strlen(payload) + 1, portMAX_DELAY);
}

static esp_err_t read_random_bytes_from_nvs(uint8_t **random_bytes, size_t *len)
{
    nvs_handle handle;
    esp_err_t err;
    *len = 0;
    if ((err = nvs_open_from_partition(RANDOM_NVS_PARTITION_NAME, RANDOM_NVS_NAMESPACE,
                                NVS_READONLY, &handle)) != ESP_OK) return ESP_FAIL;
    if ((err = nvs_get_blob(handle, RANDOM_NVS_KEY, NULL, len)) != ESP_OK) {
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }
    *random_bytes = calloc(*len, 1);
    if (*random_bytes) {
        nvs_get_blob(handle, RANDOM_NVS_KEY, *random_bytes, len);
        nvs_close(handle);
        return ESP_OK;
    }
    nvs_close(handle);
    return ESP_ERR_NO_MEM;
}

static char *custom_pop;
static app_network_pop_type_t s_cached_pop_type = POP_TYPE_NONE;
static bool s_pop_type_cached = false;

esp_err_t app_network_set_custom_pop(const char *pop)
{
    if (!pop) return ESP_ERR_INVALID_ARG;
    if (custom_pop) { free(custom_pop); custom_pop = NULL; }
    custom_pop = strdup(pop);
    return custom_pop ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t get_device_service_name(char *service_name, size_t max)
{
    uint8_t *nvs_random = NULL;
    const char *ssid_prefix = CONFIG_APP_NETWORK_PROV_NAME_PREFIX;
    size_t nvs_random_size = 0;
    if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 3) {
        uint8_t mac_addr[6];
        esp_read_mac(mac_addr, ESP_MAC_BASE);
        snprintf(service_name, max, "%s_%02x%02x%02x", ssid_prefix, mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        snprintf(service_name, max, "%s_%02x%02x%02x", ssid_prefix, nvs_random[nvs_random_size - 3],
                nvs_random[nvs_random_size - 2], nvs_random[nvs_random_size - 1]);
    }
    if (nvs_random) free(nvs_random);
    return ESP_OK;
}

char *app_network_get_device_pop(app_network_pop_type_t pop_type)
{
    if (pop_type == POP_TYPE_NONE) return NULL;
    if (pop_type == POP_TYPE_CUSTOM) {
        if (!custom_pop) return NULL;
        return strdup(custom_pop);
    }
    char *pop = calloc(1, POP_STR_SIZE);
    if (!pop) return NULL;
    if (pop_type == POP_TYPE_MAC) {
        uint8_t mac_addr[6];
        if (esp_read_mac(mac_addr, ESP_MAC_BASE) == ESP_OK) {
            snprintf(pop, POP_STR_SIZE, "%02x%02x%02x%02x", mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            return pop;
        }
    } else if (pop_type == POP_TYPE_RANDOM) {
        uint8_t *nvs_random = NULL;
        size_t nvs_random_size = 0;
        if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) == ESP_OK) && nvs_random_size >= 4) {
            snprintf(pop, POP_STR_SIZE, "%02x%02x%02x%02x", nvs_random[0], nvs_random[1], nvs_random[2], nvs_random[3]);
            free(nvs_random);
            return pop;
        }
        if (nvs_random) free(nvs_random);
    }
    free(pop);
    return NULL;
}

char *app_network_get_device_pop_default(void)
{
    if (!s_pop_type_cached) return NULL;
    return app_network_get_device_pop(s_cached_pop_type);
}

char *app_network_get_device_service_name(void)
{
    size_t buf_size = strlen(CONFIG_APP_NETWORK_PROV_NAME_PREFIX) + 1 + 6 + 1;
    char *service_name = malloc(buf_size);
    if (!service_name) return NULL;
    if (get_device_service_name(service_name, buf_size) != ESP_OK) {
        free(service_name);
        return NULL;
    }
    return service_name;
}

static void network_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
            case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
                ESP_LOGI(TAG, "Secured session established!");
                break;
            case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
                ESP_LOGE(TAG, "Received incorrect PoP or invalid security params!");
                esp_event_handler_unregister(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &network_event_handler);
                network_prov_mgr_stop_provisioning();
                esp_event_post(APP_NETWORK_EVENT, APP_NETWORK_EVENT_PROV_CRED_MISMATCH, NULL, 0, portMAX_DELAY);
                break;
        }
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
        xEventGroupSetBits(network_event_group, NETWORK_CONNECTED_EVENT);
#endif
    }
    if (event_base == NETWORK_PROV_EVENT && event_id == NETWORK_PROV_END) {
        if (prov_stop_timer) {
            esp_timer_stop(prov_stop_timer);
            esp_timer_delete(prov_stop_timer);
            prov_stop_timer = NULL;
        }
        esp_event_handler_unregister(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &network_event_handler);
        network_prov_mgr_deinit();
#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
        xEventGroupSetBits(network_event_group, NETWORK_PROV_ENDED_EVENT);
#endif
    }
}

void app_network_init(void)
{
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop, err = %x", err);
        return;
    }
    ESP_ERROR_CHECK(app_wifi_internal_init());
#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
    network_event_group = xEventGroupCreate();
#endif
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &network_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &network_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, NETWORK_PROV_END, &network_event_handler, NULL));
}

static void app_network_prov_stop(void *priv)
{
    ESP_LOGW(TAG, "Provisioning timed out. Please reboot device to restart provisioning.");
    network_prov_mgr_stop_provisioning();
    esp_event_post(APP_NETWORK_EVENT, APP_NETWORK_EVENT_PROV_TIMEOUT, NULL, 0, portMAX_DELAY);
}

static esp_err_t app_network_start_timer(void)
{
    if (prov_timeout_period == 0) return ESP_OK;
    esp_timer_create_args_t prov_stop_timer_conf = {
        .callback = app_network_prov_stop,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "app_wifi_prov_stop_tm"
    };
    if (esp_timer_create(&prov_stop_timer_conf, &prov_stop_timer) == ESP_OK) {
        esp_timer_start_once(prov_stop_timer, prov_timeout_period);
        ESP_LOGI(TAG, "Provisioning will auto stop after %d minute(s).", APP_NETWORK_PROV_TIMEOUT_PERIOD);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t app_network_start(app_network_pop_type_t pop_type)
{
    s_cached_pop_type = pop_type;
    s_pop_type_cached = true;

    char *pop = app_network_get_device_pop(pop_type);
    if ((pop_type != POP_TYPE_NONE) && (pop == NULL)) return ESP_ERR_NO_MEM;

    char service_name[strlen(CONFIG_APP_NETWORK_PROV_NAME_PREFIX) + 1 + 6 + 1];
    get_device_service_name(service_name, sizeof(service_name));
    const char *service_key = NULL;
    esp_err_t err = ESP_OK;
    bool provisioned = false;

    err = app_wifi_internal_start(pop, service_name, service_key, custom_mfg_data, custom_mfg_data_len, &provisioned);
    if (err != ESP_OK) { free(pop); return err; }

    if (!provisioned) {
#ifdef CONFIG_APP_NETWORK_PROV_TRANSPORT_BLE
        app_network_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
#else
        app_network_print_qr(service_name, pop, PROV_TRANSPORT_SOFTAP);
#endif
        app_network_start_timer();
#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
        xEventGroupSetBits(network_event_group, NETWORK_PROV_STARTED_EVENT);
#endif
    }
    free(pop);
    if (custom_mfg_data) { free(custom_mfg_data); custom_mfg_data = NULL; custom_mfg_data_len = 0; }
#if !CONFIG_APP_NETWORK_ASYNCHRONOUS_CONNECTION
    xEventGroupWaitBits(network_event_group, NETWORK_CONNECTED_EVENT, false, true, portMAX_DELAY);
#endif
    return err;
}
