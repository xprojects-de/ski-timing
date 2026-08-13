#include "webserver.h"

#include <Wire.h>
#include <Arduino.h>
#include "website.h"

Webserver::Webserver() {
}

Webserver::~Webserver() {}


esp_err_t Webserver::index_handler(httpd_req_t* req) {
  httpd_resp_set_hdr(req, "Connection", "close");
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
  httpd_resp_set_hdr(req, "Connection", "close");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");

  bool hasSentAnything = false;

  bool result = myFs->readInChunks([req, &hasSentAnything](const char* buffer, size_t length) -> bool {
    hasSentAnything = true;
    return httpd_resp_send_chunk(req, buffer, length) == ESP_OK;
  });

  if (!hasSentAnything) {
    httpd_resp_send(req, "", 0);
  } else {
    httpd_resp_send_chunk(req, NULL, 0);
  }

  return ESP_OK;
}

esp_err_t Webserver::reset_handler(httpd_req_t* req) {

  Fs* myFs = (Fs*)req->user_ctx;

  httpd_resp_set_hdr(req, "Connection", "close");

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

  config.recv_wait_timeout = 1;
  config.send_wait_timeout = 1;

  config.stack_size = 8192;
  config.core_id = 0;
  config.task_priority = 5;
  config.max_uri_handlers = 12;
  config.backlog_conn = 5;
  config.keep_alive_enable = false;
  config.max_open_sockets = 3;

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
