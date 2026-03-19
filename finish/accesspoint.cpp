#include "accesspoint.h"

#include <Wire.h>
#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

AccessPoint::AccessPoint() {
}

AccessPoint::~AccessPoint() {}

void AccessPoint::init() {

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {
    .ap = {
      .ssid = "ski-timming",
      .password = "pistenprofi",
      .ssid_len = 0,
      .channel = 1,
      .authmode = WIFI_AUTH_WPA2_PSK,
      .max_connection = 4,
    },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_netif_ip_info_t ip_info;
  esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

  if (netif) {
    esp_netif_get_ip_info(netif, &ip_info);

    Serial.print("Access Point started. SSID: ");
    Serial.println((char*)wifi_config.ap.ssid);

    Serial.print("IP-Adresse: ");

    Serial.printf("%d.%d.%d.%d\n",
                  (ip_info.ip.addr & 0xFF),
                  (ip_info.ip.addr >> 8 & 0xFF),
                  (ip_info.ip.addr >> 16 & 0xFF),
                  (ip_info.ip.addr >> 24 & 0xFF));
  }
}
