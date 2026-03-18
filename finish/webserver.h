#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <esp_err.h>
#include <esp_http_server.h>

class Webserver {

private:

public:
  Webserver();
  ~Webserver();

  void init();

  static esp_err_t index_handler(httpd_req_t* req);
  static esp_err_t data_handler(httpd_req_t* req);
};

#endif