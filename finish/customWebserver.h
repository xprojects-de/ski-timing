#ifndef CWS_H
#define CWS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "customFilesystem.h"
#include "stopwatch.h"

class CustomWebserver {

private:
  AsyncWebServer *server;
  CFs *fs_instance;
  StopWatch *sw_instance;

public:
  CustomWebserver();
  ~CustomWebserver();

  void init(CFs *fs_inst, StopWatch *sw_inst);

  void index_handler(AsyncWebServerRequest *request);
  void data_handler(AsyncWebServerRequest *request);
  void reset_handler(AsyncWebServerRequest *request);
  void discard_handler(AsyncWebServerRequest *request);
  void status_handler(AsyncWebServerRequest *request);
  void enableContinuousMode_handler(AsyncWebServerRequest *request);
  void disableContinuousMode_handler(AsyncWebServerRequest *request);
};

#endif