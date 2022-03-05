# Arduino CO2 sensor with WiFi , MQTT and charts on webserver

Homebrew (DIY) IoT CO2 sensor. Uses ESP32 module and MH-Z19C (or similar) sensor. Arduino platform.

Features:
- WiFi connectivity (todo: with WPS)
- MQTT transmit to SmartNest.cz (using Thermometer template)
- OLED display
- Web server with live charts
- CO2 sensor is connected to UART 2 on ESP32 chip

## Hardware

- Any ESP32 module

Tested on following DEV boards (you must select different pins for OLED)
- https://www.aliexpress.com/item/4000065217965.html
- https://www.aliexpress.com/item/32847022581.html
    
- SSD1306 based display (currently used 128x62) over I2C
- MH-19C sensor for CO2

Tested with sensors:
- https://www.aliexpress.com/item/1005002994757073.html
- https://www.aliexpress.com/item/1005001947070873.html

## Libraries required

- PubSub client https://pubsubclient.knolleary.net/
- Arduino_JSON https://github.com/arduino-libraries/Arduino_JSON
- Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306
- Adafruit GFX https://github.com/adafruit/Adafruit-GFX-Library
- Arduino JSON https://arduinojson.org/
- ESP32 AsyncTCP https://github.com/me-no-dev/AsyncTCP
- ESP32 AsyncWebServer https://github.com/me-no-dev/ESPAsyncWebServer
- MH-Z Sensors - modified, included with project (original: https://github.com/tobiasschuerg/MH-Z-CO2-Sensors)

## Sources

- https://www.winsen-sensor.com/sensors/co2-sensor/mh-z19b.html
- https://revspace.nl/MHZ19
- https://github.com/tobiasschuerg/MH-Z-CO2-Sensors/tree/master/examples/MH-Z19B
- https://randomnerdtutorials.com/esp32-plot-readings-charts-multiple/
- https://randomnerdtutorials.com/esp32-built-in-oled-ssd1306/
- https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
