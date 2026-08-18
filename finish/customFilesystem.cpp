#include "customFilesystem.h"
#include <LittleFS.h>

CFs::CFs() {
  _initialized = false;
  _lastId = 0;
}

CFs::~CFs() {}

void CFs::init() {

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed!");
    _initialized = false;
    return;
  }

  _initialized = true;
  Serial.println("LittleFS successfully mounted");

  _lastId = getLastId();
}

bool CFs::isInit() {
  return _initialized;
}

bool CFs::append(float time) {
  if (!_initialized) return false;

  _lastId++;

  File f = LittleFS.open("/times.csv", FILE_APPEND);
  if (!f) {
    _lastId--;
    return false;
  }

  f.printf("%d,%.2f\n", _lastId, time);
  f.close();
  return true;
}

int CFs::getLastId() {
  if (!LittleFS.exists("/times.csv")) return 0;

  File f = LittleFS.open("/times.csv", FILE_READ);
  if (!f) return 0;

  int lastId = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    int id;
    if (sscanf(line.c_str(), "%d,", &id) == 1) {
      lastId = id;
    }
  }

  f.close();
  return lastId;
}

String CFs::readAll() {
  if (!_initialized) return "FS not init";
  if (!LittleFS.exists("/times.csv")) return "";

  File f = LittleFS.open("/times.csv", FILE_READ);
  if (!f) return "";

  String content = f.readString();
  f.close();

  return content;
}

bool CFs::clear() {
  if (!_initialized) return false;

  File f = LittleFS.open("/times.csv", FILE_WRITE);
  if (!f) {
    Serial.println("error clear file!");
    return false;
  }

  f.close();
  _lastId = 0;

  Serial.println("Clear file successful.");
  return true;
}

bool CFs::readInChunks(std::function<bool(const char* buffer, size_t length)> chunkCallback) {
  if (!_initialized) return false;
  if (!LittleFS.exists("/times.csv")) return false;

  File f = LittleFS.open("/times.csv", FILE_READ);
  if (!f) return false;

  const size_t bufferSize = 512;
  char buffer[bufferSize];

  bool success = true;
  int chunkCount = 0;
  unsigned long lastYield = millis();

  while (f.available()) {
    size_t bytesRead = f.readBytes(buffer, bufferSize);

    if (bytesRead > 0) {
      if (!chunkCallback(buffer, bytesRead)) {
        success = false;
        break;
      }

      chunkCount++;
      unsigned long now = millis();
      if (chunkCount >= 2 || (now - lastYield) >= 20) {
        yield();
        chunkCount = 0;
        lastYield = now;
      }
    }
  }

  f.close();
  return success;

}