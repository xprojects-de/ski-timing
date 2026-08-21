#ifndef GPS_TIME_H
#define GPS_TIME_H

#include <Arduino.h>
#include "HT_TinyGPS++.h"

class GpsTime {
private:
  TinyGPSPlus gps;
  HardwareSerial gpsSerial;
  uint8_t pwrPin;

public:
  GpsTime(uint8_t rxPin = 38, uint8_t txPin = 39, uint8_t pwr_pin = 34);

  void init();
  void update();
  bool hasFix();
  bool getTimeString(char* buffer, size_t len);
  bool getDateString(char* buffer, size_t len);
};

#endif