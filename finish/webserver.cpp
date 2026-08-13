#include "webserver.h"

#include <Wire.h>
#include <Arduino.h>
#include "website.h"

Webserver::Webserver() {
}

Webserver::~Webserver() {}


esp_err_t Webserver::index_handler(httpd_req_t* req) {
  httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t Webserver::data_handler(httpd_req_t* req) {
  Fs* myFs = (Fs*)req->user_ctx;

  if (myFs == NULL) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No FS Instance");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "text/plain");

  bool result = myFs->readInChunks([req](const char* buffer, size_t length) -> bool {
    return httpd_resp_send_chunk(req, buffer, length) == ESP_OK;
  });

  httpd_resp_send_chunk(req, NULL, 0);

  if (result) {
    Serial.println("Data successfully streamed using readInChunks().");
  } else {
    Serial.println("Stream aborted or empty file.");
  }

  return ESP_OK;
}

esp_err_t Webserver::reset_handler(httpd_req_t* req) {

  Fs* myFs = (Fs*)req->user_ctx;

  if (myFs != NULL && myFs->clear()) {
    httpd_resp_send(req, "Data deleted. <a href='/'>Back</a>", HTTPD_RESP_USE_STRLEN);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error");
  }
  return ESP_OK;
}

void Webserver::init(Fs* fs_instance) {

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.lru_purge_enable = true;
  config.recv_wait_timeout = 10;
  config.send_wait_timeout = 10;
  config.max_open_sockets = 4; 

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
      .user_ctx = fs_instance
    };
    httpd_register_uri_handler(server, &uri_data);

    httpd_uri_t uri_reset = {
      .uri = "/reset",
      .method = HTTP_GET,
      .handler = Webserver::reset_handler,
      .user_ctx = fs_instance
    };
    httpd_register_uri_handler(server, &uri_reset);

    Serial.println("Webserver started!");
  }
}
