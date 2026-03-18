#include "accesspoint.h"

#include <Wire.h>
#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Konstruktor: Initialisiert die Werte
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

  // 2. Netzwerk-Interface & Event-Loop
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  // 3. WiFi Konfiguration
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

  Serial.println("Access Point gestartet. SSID: Ski-Zeitnahme-S3");
}
