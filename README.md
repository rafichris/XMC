X-MAS LED CHANNEL DRIVER (XMC) Firmware
=====================

This is the Arduino firmware for the ESP8266 based XMC.  The XMC is a small wireless E1.31 sACN LED stips controller designed to control up to 16 strips. The master with the ESP8266 can be extended with several slave boards to get with each slave 16 additional output ports. The slave boards are controlled by the masters I2C lines. Channel limitations are mostly based upon your desired refresh rate.
Keep in mind, that the sum of all current shall be provided by the master with its power supply.

The firmware has a similar WEB UI than the [ESPixelStick](https://github.com/forkineye/ESPixelStick) Firmware.
The idea of controlling LED strips with an [PCA9685](https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf) is figured in RichardS [post](http://www.esp8266.com/viewtopic.php?f=152&t=8814)

Requirements
------------
Along with the Arduino IDE, you'll need the following software to build this project:
- [Adruino for ESP8266](https://github.com/esp8266/Arduino) - Arduino core for ESP8266
- [Arduino ESP8266 Filesystem Uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) - Arduino plugin for uploading files to SPIFFS
- [gulp](http://gulpjs.com/) - Build system to process web sources.  Optional, but recommended.  Refer to the html [README](html/README.md) for more information.

The following libraries are required:
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) - Arduino JSON Library
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) - Asynchronous TCP Library
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Asynchronous Web Server Library

Important Notes on Compiling and Flashing
-----------------------------------------
- Web pages **must** be processed, placed into ```data/www```, and uploaded with the upload plugin. Gulp will process the pages and put them in ```data/www``` for you. Refer to the html [README](html/README.md) for more information.
- In order to use the upload plugin, the ESP8266 **must** be placed into programming mode and the Arduino serial monitor **must** be closed.
- ESP-01 modules **must** be configured for 1M flash and 128k SPIFFS within the Arduino IDE for OTA updates to work.
- For best performance, set the CPU frequency to 160MHz (Tools->CPU Frequency).  You may experience lag and other issues if running at 80MHz.

Supported Outputs
-----------------
The XMC firmware can generate the following outputs from incoming E1.31 streams, however your hardware must support the physical interface.
#### LED Strips
- 12V LED Strips

Resources
---------
- Firmware: http://github.com/rafichris/XMC
- Hardware: (in work)

Credits
-------
- Dieter Reinhardt an Markus Felinger for inspiration.
- The people at http://forkineye.com/ESPixelStick and http://github.com/forkineye/ESPixelStick for the fancy WEB UI.
