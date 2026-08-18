#include "accesspoint.h"

#include <WiFi.h>
#include <Arduino.h>

const char *ssid = "time-control";
const char *password = "time-control";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AccessPoint::AccessPoint() {}
AccessPoint::~AccessPoint() {}

void AccessPoint::init() {

  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("error at AP configuration!");
  }

  if (WiFi.softAP(ssid, password)) {

    Serial.println("Access Point started!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.softAPIP());

  } else {
    Serial.println("error AP-Startup!");
  }
}
