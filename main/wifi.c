#include <nvs_flash.h>
#include <stdio.h>
#include <setjmp.h>

#include <esp_http_client.h>
#include <esp_wifi.h>
#include <esp_log.h>

#include "wifi.h"

static esp_http_client_handle_t s_http_client = NULL;  

static const char* TAG = "wifi";
EventGroupHandle_t s_wifi_event_group;
static int s_wifi_retry_num = 0;

static bool wifi_initialized = false;
static bool wifi_initializing = false;
static SemaphoreHandle_t wifi_mutex;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (s_wifi_retry_num < WIFI_MAX_RETRY) 
        {
            esp_wifi_connect();
            s_wifi_retry_num++;
            ESP_LOGI(TAG, "connecting...");
        } else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connecting failed");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init() {
    if (wifi_initialized || wifi_initializing) 
    {
        ESP_LOGI(TAG, "wifi already initialized");
        return ESP_OK;
    }

    wifi_initializing = true;

    esp_err_t rv = nvs_flash_init();
    if (rv == ESP_ERR_NVS_NO_FREE_PAGES || rv == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        rv = nvs_flash_erase();
        if (rv != ESP_OK) {
            ESP_LOGE(TAG, "failed to erase NVS");
            return rv;
        }

        rv = nvs_flash_init();
        if (rv != ESP_OK) {
            ESP_LOGE(TAG, "failed to init NVS");
            return rv;
        }
    }

    if (rv != ESP_OK) {
        return rv;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "failed to create event group");
        return ESP_FAIL;
    }

    rv = esp_netif_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to init esp-netif");
        return rv;
    }

    rv = esp_event_loop_create_default();
    if (rv != ESP_OK) 
    {
        ESP_LOGE(TAG, "failed to create default event loop");
        return rv;
    }
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    rv = esp_wifi_init(&cfg);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize wifi");
        return rv;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    rv = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to register wifi event handler");
        return rv;
    }

    rv = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to register IP event handler");
        return rv;
    }

    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    rv = esp_wifi_set_mode(WIFI_MODE_STA);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to set wifi mode");
        return rv;
    }

    rv = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to set wifi configuration");
        return rv;
    }

    rv = esp_wifi_start();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "failed to start wifi");
        return rv;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: <%s>", WIFI_SSID);
        wifi_initialized = true;
        wifi_initializing = false;
        return ESP_OK;
    } 
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID: <%s>", WIFI_SSID);
        wifi_initializing = false;
        return ESP_FAIL;
    } 
    else {
        ESP_LOGE(TAG, "Unexpected error occurred");
        wifi_initializing = false;
        return ESP_FAIL;
    }
}