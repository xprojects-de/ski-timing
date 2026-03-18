#include "webserver.h"

#include <Wire.h>
#include <Arduino.h>
#include "website.h"

// Konstruktor: Initialisiert die Werte
Webserver::Webserver() {
}

Webserver::~Webserver() {}


esp_err_t Webserver::index_handler(httpd_req_t* req) {
  httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t Webserver::data_handler(httpd_req_t* req) {

  int luckyNumber = random(10, 100);

  char resp[100];
  snprintf(resp, sizeof(resp), "%d,10.80\n2,10.90\n3,%d", luckyNumber);

  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

void Webserver::init() {

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&server, &config) == ESP_OK) {

    httpd_uri_t uri_index = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = Webserver::index_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_index);

    httpd_uri_t uri_data = {
      .uri = "/data",
      .method = HTTP_GET,
      .handler = Webserver::data_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_data);

    Serial.println("Webserver gestartet!");
  }
}
