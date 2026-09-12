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
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
  response->addHeader("Cache-Control", "max-age=3600");
  request->send(response);
}

void CustomWebserver::data_handler(AsyncWebServerRequest *request) {
  if (fs_instance == nullptr) {
    request->send(500, "text/plain", "No FS Instance");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");

  bool success = fs_instance->readInChunks(
    [response](const char *data, size_t length) -> bool {
      response->write((const uint8_t *)data, length);
      return true;
    });

  if (!success) {
    response->print("Error reading data");
  }

  request->send(response);
}

void CustomWebserver::reset_handler(AsyncWebServerRequest *request) {
  if (fs_instance != nullptr && sw_instance != nullptr && sw_instance->reset() && fs_instance->clear()) {

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "Data deleted. <a href='/'>Back</a>");
    request->send(response);
  } else {

    AsyncWebServerResponse *response = request->beginResponse(500, "text/plain", "Error");
    request->send(response);
  }
}

void CustomWebserver::status_handler(AsyncWebServerRequest *request) {
  if (sw_instance == nullptr) {
    request->send(500, "text/plain", "No SW instance");
    return;
  }
  const char *mode = sw_instance->isContinuousMode() ? "continuous" : "normal";
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", mode);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  request->send(response);
}

void CustomWebserver::discard_handler(AsyncWebServerRequest *request) {

  if (sw_instance == nullptr) {
    request->send(500, "text/plain", "No SW instance");
    return;
  }
  if (sw_instance->isContinuousMode()) {
    request->send(400, "text/plain", "Not applicable in continuous mode");
    return;
  }
  if (sw_instance->discardOldestStart()) {
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Queue empty");
  }
}

void CustomWebserver::enableContinuousMode_handler(AsyncWebServerRequest *request) {
  if (sw_instance != nullptr) {

    sw_instance->setContinuousMode(true);
    sw_instance->reset();
    if (fs_instance != nullptr) fs_instance->clear();

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "");
    request->send(response);

  } else {

    AsyncWebServerResponse *response = request->beginResponse(500, "text/plain", "");
    request->send(response);
  }
}

void CustomWebserver::disableContinuousMode_handler(AsyncWebServerRequest *request) {
  if (sw_instance != nullptr) {

    sw_instance->setContinuousMode(false);
    sw_instance->reset();
    if (fs_instance != nullptr) fs_instance->clear();

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "");
    request->send(response);

  } else {

    AsyncWebServerResponse *response = request->beginResponse(500, "text/plain", "");
    request->send(response);
  }
}

void CustomWebserver::ping_handler(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "OK");
}

void CustomWebserver::init(CFs *fs_inst, StopWatch *sw_inst) {

  fs_instance = fs_inst;
  sw_instance = sw_inst;

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

  server->on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->status_handler(request);
  });

  server->on("/discard", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->discard_handler(request);
  });

  server->on("/enableContinuousMode", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->enableContinuousMode_handler(request);
  });

  server->on("/disableContinuousMode", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->disableContinuousMode_handler(request);
  });

  server->on("/ping", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->ping_handler(request);
  });

  server->onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("404: %s\n", request->url().c_str());
    AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "");
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server->begin();
  Serial.println("Webserver started!");
}