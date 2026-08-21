#include "gpsTime.h"

GpsTime::GpsTime(uint8_t rxPin, uint8_t txPin, uint8_t pwr_pin)
  : gpsSerial(1), pwrPin(pwr_pin) {
}

void GpsTime::init() {
  pinMode(pwrPin, OUTPUT);
  digitalWrite(pwrPin, LOW);  // Power ON (active low)
  delay(100);

  gpsSerial.begin(9600, SERIAL_8N1, 38, 39);  // RX, TX
}

void GpsTime::update() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

bool GpsTime::hasFix() {
  return gps.location.isValid() && gps.time.isValid() && gps.date.isValid();
}

bool GpsTime::getTimeString(char* buffer, size_t len) {
  if (!gps.time.isValid()) return false;
  snprintf(buffer, len, "%02d:%02d:%02d",
           gps.time.hour(), gps.time.minute(), gps.time.second());
  return true;
}

bool GpsTime::getDateString(char* buffer, size_t len) {
  if (!gps.date.isValid()) return false;
  snprintf(buffer, len, "%02d.%02d.%04d",
           gps.date.day(), gps.date.month(), gps.date.year());
  return true;
}