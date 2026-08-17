#ifndef CWS_H
#define CWS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "customFilesystem.h"

class CustomWebserver {

private:
  AsyncWebServer* server;
  CFs* fs_instance;

public:
  CustomWebserver();
  ~CustomWebserver();

  void init(CFs *fs_instance);

  void index_handler(AsyncWebServerRequest *request);
  void data_handler(AsyncWebServerRequest *request);
  void reset_handler(AsyncWebServerRequest *request);
};

#endif