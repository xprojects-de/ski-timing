#include "accesspoint.h"

#include <Wire.h>
#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

AccessPoint::AccessPoint() {}
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

  esp_netif_t* netif = esp_netif_create_default_wifi_ap();

  esp_netif_dhcps_stop(netif);
  esp_netif_ip_info_t ip_info;
  IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
  IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
  IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
  esp_netif_set_ip_info(netif, &ip_info);
  esp_netif_dhcps_start(netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.ampdu_rx_enable = 0;
  cfg.ampdu_tx_enable = 0;
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {};
  strcpy((char*)wifi_config.ap.ssid, "ski-timing");
  wifi_config.ap.ssid_len = strlen("ski-timing");
  wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  wifi_config.ap.channel = 1;
  wifi_config.ap.max_connection = 8;
  wifi_config.ap.beacon_interval = 60;
  wifi_config.ap.dtim_period = 1;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

  esp_wifi_set_max_tx_power(84); // 21 dBm
}
