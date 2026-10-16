# ski-timing

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Firmware: Arduino / ESP32](https://img.shields.io/badge/Firmware-Arduino%20%2F%20ESP32-blue.svg)](start)

Firmware for a LoRa-based start/finish timing system for ski races and similar club
competitions, running on Heltec WiFi LoRa 32 boards. A **start** unit and a **finish** unit talk
to each other over LoRa: the start unit synchronizes its clock with the finish unit and sends a
timestamp the moment the start button (or gate) is triggered, the finish unit timestamps the
finish trigger the same way and keeps a queue of elapsed times. Everything runs standalone on
battery power, with an OLED status display and no need for a network or the internet.

The finish unit's built-in web page (its own WiFi access point) can download the recorded times
as a simple `id,time` CSV. That raw export is exactly the kind of timing-device output
[Alpdesk Time Control](https://github.com/xprojects-de/alpdesk-time-control) — our sister project
for running the rest of a club competition (start lists, scoring, PDFs, live results) — can take
in through its CSV import with column mapping.

See [CONTRIBUTING.md](CONTRIBUTING.md) for how to build and submit changes, and
[SECURITY.md](SECURITY.md) for the intended operating environment and how to report a
vulnerability. This project follows the [Code of Conduct](CODE_OF_CONDUCT.md).

## Setup

### Arduino IDE

Download and install https://www.arduino.cc/en/software/

Unter preferences, add following URLs under  the "Additional Boards Manager URLs"
- [https://espressif.github.io/arduino-esp32/package_esp32_index.json](https://espressif.github.io/arduino-esp32/package_esp32_index.json)
- [https://resource.heltec.cn/download/package_heltec_esp32_index.json](https://resource.heltec.cn/download/package_heltec_esp32_index.json)

Under Board-Maanger install following:
- esp32 by espressif
- Healtec esp32 series Arduino Development environment (https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series)

Under Library Manager, install following libraries
- Heltec ESP32 Dev-Boards
- ESPAsyncWebServer (https://github.com/ESP32Async/ESPAsyncWebServer)
  - Install via Library Manager or manually from GitHub
  - This also requires AsyncTCP library (https://github.com/me-no-dev/AsyncTCP)


Select the right Board and settings under Tools (see image). This project targets the **Heltec
WiFi LoRa 32 (V4)** board; the settings used are:

| Setting | Value |
|---|---|
| Board | `WiFi LoRa 32(V4)` |
| USB CDC On Boot | `Enabled` |
| CPU Frequency | `240MHz (WiFi)` |
| Core Debug Level | `Info` |
| USB DFU On Boot | `Disabled` |
| Erase All Flash Before Sketch Upload | `Disabled` |
| Events Run On | `Core 1` |
| Flash Mode | `QIO 80MHz` |
| Flash Size | `16MB (128Mb)` |
| JTAG Adapter | `Disabled` |
| Arduino Runs On | `Core 1` |
| LoRaWan DevEUI | `CUSTOM` |
| LoRaWan Preamble Length | `8 (default)` |
| LoRaWan Region | `REGION_EU868` |
| LoRaWan Debug Level | `None` |
| USB Firmware MSC On Boot | `Disabled` |
| Partition Scheme | `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)` |
| Upload Mode | `UART0 / Hardware CDC` |
| Upload Speed | `115200` |
| USB Mode | `Hardware CDC and JTAG` |
| Zigbee Mode | `Disabled` |

`LoRaWan Region` has to match the frequency band used in your country (`REGION_EU868` for
Europe); `Port` depends on your machine — check *Tools → Port* for the entry that appears when the
board is plugged in.

![Board Settings](settings/settings.png)


### esptool

Sometimes it is necessary to reset or reflash the firmware using esptool.

Download and install https://github.com/espressif/esptool/releases

To run and reset the board runn following:

```bash 
### /dev/cu.usbmodem1101 is the port of the board, check your system for the right one
./esptool --no-stub --port /dev/cu.usbmodem1101 flash-id
```

### software

"start" folder contains the code for the start gate, "finish" folder contains the code for the finish gate. 
Both can be flashed to the same board, but they have different functionality. 
The code is written in C++ and uses the Arduino framework.
