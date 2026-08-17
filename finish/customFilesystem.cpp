#include "customFilesystem.h"

#include <esp_littlefs.h>

CFs::CFs() {
  _initialized = false;
  _lastId = 0;
}

CFs::~CFs() {}

void CFs::init() {

  esp_vfs_littlefs_conf_t conf = {
    .base_path = "/littlefs",
    .partition_label = "spiffs",
    .format_if_mount_failed = true,
    .dont_mount = false
  };

  esp_err_t ret = esp_vfs_littlefs_register(&conf);
  _initialized = ret == ESP_OK;

  if (ret != ESP_OK) {

    if (ret == ESP_FAIL) {
      Serial.println("error mounting!");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      Serial.println("no partition found!");
    } else {
      Serial.printf("error initialize (%s)\n", esp_err_to_name(ret));
    }

    return;
  }

  Serial.println("LittleFS successfully mountet under /littlefs");

  _lastId = getLastId();

}

bool CFs::isInit() {
  return _initialized;
}

bool CFs::append(float time) {

  if (!_initialized) return false;

  _lastId++;

  FILE* f = fopen("/littlefs/times.csv", "a");
  if (f == NULL) {
    _lastId--;
    return false;
  }

  fprintf(f, "%d,%.2f\n", _lastId, time);
  fclose(f);
  return true;
}

int CFs::getLastId() {

  FILE* f = fopen("/littlefs/times.csv", "r");
  if (f == NULL) return 0;

  int lastId = 0;
  char line[128];
  while (fgets(line, sizeof(line), f)) {
    int id;
    if (sscanf(line, "%d,", &id) == 1) {
      lastId = id;
    }
  }

  fclose(f);
  return lastId;
}

String CFs::readAll() {

  if (!_initialized) return "FS not init";

  struct stat st;
  if (stat("/littlefs/times.csv", &st) != 0) {
    return "";
  }

  FILE* f = fopen("/littlefs/times.csv", "r");
  if (f == NULL) return "";

  String content = "";
  char line[64];
  while (fgets(line, sizeof(line), f)) {
    content += line;
  }

  fclose(f);

  return content;
}

bool CFs::clear() {

  if (!_initialized) return false;

  FILE* f = fopen("/littlefs/times.csv", "w");

  if (f == NULL) {
    Serial.println("error clear file!");
    return false;
  }

  fclose(f);

  _lastId = 0;

  Serial.println("Clear file successful.");

  return true;
}

bool CFs::readInChunks(std::function<bool(const char* buffer, size_t length)> chunkCallback) {

  if (!_initialized) return false;

  FILE* f = fopen("/littlefs/times.csv", "r");
  if (f == NULL) return false;

  size_t bufferSize = 512;
  char* buffer = (char*)malloc(bufferSize);
  if (buffer == NULL) {
    fclose(f);
    return false;
  }

  size_t bytesRead;
  bool success = true;
  int chunkCount = 0;
  unsigned long lastYield = millis();

  while ((bytesRead = fread(buffer, 1, bufferSize, f)) > 0) {

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

  free(buffer);
  fclose(f);
  
  return success;

}
