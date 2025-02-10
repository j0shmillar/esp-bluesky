#include "nvs_flash.h"
#include "sdkconfig.h"

#include "esp_wifi.h"
#include "esp_log.h"

#include "jansson.h"
#include "bluesky.h"
#include "wifi.h"

#define BSKY_HANDLE CONFIG_BSKY_HANDLE
#define BSKY_PASSWORD CONFIG_BSKY_PASSWORD

void app_main() {
    esp_err_t result = wifi_init();
    if (result == ESP_OK) {
        ESP_LOGI("wifi", "initialized\n");
    } 
    else {
        ESP_LOGE("wifi", "init failed\n");
        return;
    }

    ESP_LOGI("bsky", "initializing client...");

    int ret = bs_client_init(BSKY_HANDLE, BSKY_PASSWORD, NULL);
    if (ret > 0) {
        ESP_LOGE("bsky", "failed to login");
        return;
    }

    ESP_LOGI("bsky", "posting...");
    bs_client_response_t *res = bs_client_post("{\"text\": \"Hello from ESP32!\"}");
    if (res) {
        printf("%s\n", res->resp);
        bs_client_response_free(res);
    } 
    else {
        ESP_LOGE("bsky", "failed to post");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); 
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_err_t rv = wifi_deinit();
    if (rv != ESP_OK) {
        ESP_LOGE("wifi", "failed to deinitialize");
    }
}
