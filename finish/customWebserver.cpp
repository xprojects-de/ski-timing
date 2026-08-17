#include "customWebserver.h"

#include <Wire.h>
#include <Arduino.h>
#include "website.h"

CustomWebserver::CustomWebserver() {
  server = nullptr;
  fs_instance = nullptr;
}

CustomWebserver::~CustomWebserver() {
  if (server != nullptr) {
    delete server;
  }
}

void CustomWebserver::index_handler(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
}

void CustomWebserver::data_handler(AsyncWebServerRequest *request) {

  if (fs_instance == nullptr) {
    request->send(500, "text/plain", "No FS Instance");
    return;
  }

  String data = fs_instance->readAll();

  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", data);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  request->send(response);

}

void CustomWebserver::reset_handler(AsyncWebServerRequest *request) {
  if (fs_instance != nullptr && fs_instance->clear()) {
    request->send(200, "text/html", "Data deleted. <a href='/'>Back</a>");
  } else {
    request->send(500, "text/plain", "Error");
  }
}

void CustomWebserver::init(CFs* fs_inst) {
  fs_instance = fs_inst;
  server = new AsyncWebServer(80);

  server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->index_handler(request);
  });

  server->on("/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->data_handler(request);
  });

  server->on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->reset_handler(request);
  });

  server->begin();
  Serial.println("Webserver started!");
  
}
