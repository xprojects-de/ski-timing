#include "fs.h"

#include <esp_littlefs.h>

Fs::Fs() {
  _initialized = false;
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
}

bool Fs::isInit() {
  return _initialized;
}

bool Fs::append(float time) {

  if (!_initialized) return false;

  FILE* f = fopen("/littlefs/times.csv", "a");
  if (f == NULL) return false;

  fprintf(f, "%.2f\n", time);
  fclose(f);
  return true;
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

  Serial.println("Clear file successful.");

  return true;
}