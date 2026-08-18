# ski-timing

## Setup

### Arduino IDE

Download and install https://www.arduino.cc/en/software/

Unter preferences, add following URLs under  the "Additional Boards Manager URLs"
- [https://resource.heltec.cn/download/package_heltec_esp32_index.json](https://espressif.github.io/arduino-esp32/package_esp32_index.json)
- [https://dl.espressif.com/dl/package_esp32_index.json](https://resource.heltec.cn/download/package_heltec_esp32_index.json)

Under Board-Maanger install following:
- esp32 by espressif
- Healtec esp32 series Arduino Development environment

Under Library Manager, install following libraries
- Heltec ESP32 Dev-Boards
- ESPAsyncWebServer (https://github.com/ESP32Async/ESPAsyncWebServer)
  - Install via Library Manager or manually from GitHub
  - This also requires AsyncTCP library (https://github.com/me-no-dev/AsyncTCP)


Select the right Board and settings under Tools (see image)

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
