/*
 * Author: Aljaz Ogrin
 * Project: OLED CLOCK
 * Hardware: ESP32
 * File description: Global configuration for the complete project
 */
 
#ifndef GLOBAL_DEFINES_H_
#define GLOBAL_DEFINES_H_

#include <stdint.h>
#include <Arduino.h>

// ************ General config *********************

//#define DEBUG_OUTPUT

#define DEVICE_NAME       "OLED-clock-CO2"
#define FIRMWARE_VERSION  "aly-fly OLED clock v3"
#define NIGHT_TIME  21 // dim displays at 9 pm 
#define DAY_TIME     7 // full brightness after 7 am
#define NTP_INTERVAL_SEC  (11 * 60 * 60)   // every 11 hours


// ************ WiFi config *********************
#define ESP_WPS_MODE      WPS_TYPE_PBC  // push-button
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "OLED clock"

#define WIFI_CONNECT_TIMEOUT_SEC  40
#define WIFI_RECONNECT_RETRY_MIN  2  // if WiFi fails, trex to reconnect every X minutes

#define GEOLOCATION_ENABLED    // enable after creating an account and copying Geolocation API below:
#define GEOLOCATION_API_KEY "=====  your api  key  ====="  // free for 5k loopkups per month. Get yours on https://www.abstractapi.com/ (login) --> https://app.abstractapi.com/api/ip-geolocation/tester (key)

// ************ MQTT *********************

#define MQTT_BROKER "smartnest.cz"                    // Broker host
#define MQTT_PORT 1883                                // Broker port
#define MQTT_USERNAME "=====  your username  ====="                      // Username from Smartnest
#define MQTT_PASSWORD "=====  your api  key  ====="                      // Password from Smartnest (or API key)
#define MQTT_CLIENT "=====  your device  id  ====="                       // Device Id from smartnest
#define MQTT_STATUS_REPORT_SEC 30


#define SHOW_TIME_SEC 4
#define SHOW_SENSOR_SEC 9
#define SCROLL_DELAY  0   // typ: 15 for SPI, 1 for I2C
#define SCROLL_PIXELS  3  // typ:  1 for SPI, 4 for I2C

#define WEBSERVER_SEND_TO_CHARTS  30

// ************ Hardware definitions *********************

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//#define OLED_SPI
  #define OLED_CLK   19
  #define OLED_MOSI  18
  #define OLED_DC    17
  #define OLED_CS    16
//  #define OLED_RESET  5
  #define OLED_MISO   4  // not used, must be defined for hardware SPI
  #define OLED_HARDWARE_SPI  // daefault is VSPI on ESP32

#define OLED_I2C
/*  
  // White PCB board  // Start I2C Communication SDA = 4 and SCL = 15
  // https://www.aliexpress.com/item/32847022581.html
  // Heltec ESP32 Oled Wifi kit 32
  #define OLED_RESET     16 // Reset pin # (or -1 if sharing Arduino reset pin)
  #define OLED_SCL       15
  #define OLED_SDA        4
*/  
  // Balck PCB board // https://www.aliexpress.com/item/4000065217965.html
  #define OLED_RESET     -1
  #define OLED_SCL        4
  #define OLED_SDA        5

  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// ************ CO2 sensor *********************
#define READ_CO2_SENSOR_SEC  3

// pin for ESP32 uart2 reading
#define MH_Z19_RX 25
#define MH_Z19_TX 26

#define CO2_SERIAL_DEBUG  false

#endif /* GLOBAL_DEFINES_H_ */
