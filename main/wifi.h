#ifndef WIFI_H
#define WIFI_H

#include <esp_http_client.h>
#include <esp_event.h>
#include <esp_err.h>

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS
#define WIFI_MAX_RETRY 3
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

extern EventGroupHandle_t s_wifi_event_group;

void http_init();
esp_err_t wifi_init();
esp_err_t wifi_deinit();
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t wifi_send_audio(const int16_t* const samples, size_t count);
esp_err_t wifi_send_image(const uint8_t* image_data, size_t length);

#endif