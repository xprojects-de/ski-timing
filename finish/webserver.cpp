#include "webserver.h"

#include <Wire.h>
#include <Arduino.h>

// Konstruktor: Initialisiert die Werte
Webserver::Webserver() {
}

Webserver::~Webserver() {}


esp_err_t Webserver::get_data_handler(httpd_req_t* req) {
  const char* resp = "Lauf 1: 42.5s\nLauf 2: 41.2s";  // Hier liest du später die CSV
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

void Webserver::init() {

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t uri_get = {
      .uri = "/data",
      .method = HTTP_GET,
      .handler = Webserver::get_data_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_get);
  }
}
