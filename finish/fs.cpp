#include "fs.h"

#include <esp_littlefs.h>

Fs::Fs() {
  _initialized = false;
  _lastId = 0;
}

Fs::~Fs() {}

void Fs::init() {

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

bool Fs::isInit() {
  return _initialized;
}

bool Fs::append(float time) {

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

int Fs::getLastId() {

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

String Fs::readAll() {

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

bool Fs::clear() {

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

bool Fs::readInChunks(std::function<bool(const char* buffer, size_t length)> chunkCallback) {

  if (!_initialized) return false;

  FILE* f = fopen("/littlefs/times.csv", "r");
  if (f == NULL) return false;

  char buffer[512];
  size_t bytesRead;
  bool success = true;

  while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {

    if (!chunkCallback(buffer, bytesRead)) {
      success = false;
      break; 
    }

    yield();

  }

  fclose(f);
  
  return success;

}
