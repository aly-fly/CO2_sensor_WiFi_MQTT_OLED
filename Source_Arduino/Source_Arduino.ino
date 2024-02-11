/*
 * Author: Aljaz Ogrin
 * Project: OLED CLOCK
 * Original location: https://github.com/aly-fly/
 * Hardware: ESP32
 */

#include <stdint.h>
#include "_USER_DEFINES.h"
#include "Clock.h"
#include "WiFi_WPS.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#ifdef OLED_SPI
  #include <SPI.h>
  #ifdef OLED_HARDWARE_SPI
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);
  #else
    // Declaration for SSD1306 display connected using software SPI
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #endif 
#endif

#ifdef OLED_I2C
  #include <Wire.h>
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#ifdef ONE_WIRE_BUS_PIN
  #include "DallasTemp2.h"
#endif

Clock uclock;
bool CurrentDisplayClock = true;  // switch between temperature and clock display
uint32_t LastTimeDisplayChanged = 0;
String ClockTxt;
String SensorTxt;
uint8_t DisplayOFF = 0; // possibility to turn off the display (for example at night) over MQTT

void oledBegin(void) {
  // Setup OLED

#ifdef OLED_SPI
#ifdef OLED_HARDWARE_SPI 
  SPI.begin(OLED_CLK, 4, OLED_MOSI, OLED_CS); // SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
#endif

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
#endif // SPI

#ifdef OLED_I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(800000);  // 800 kHz https://github.com/espressif/arduino-esp32/issues/4505
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
#endif

  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(0, 0);     // Start at top-left corner
  display.display();

  // test Draw a single pixel in white
//  display.drawPixel(20, 20, SSD1306_WHITE);
//  display.display();

}


#include <WiFi.h> // ESP32
#include "esp_wps.h"
#include "IPGeolocation_AO.h"
#include "WiFi_WPS.h"

WifiState_t WifiState = disconnected;
uint32_t LastTimeWifiReconnect = 0;
static esp_wps_config_t wps_config;
double GeoLocTZoffset = 0;


void wpsInitConfig(){
//  wps_config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  wps_config.wps_type = ESP_WPS_MODE;
  strcpy(wps_config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(wps_config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(wps_config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(wps_config.factory_info.device_name, DEVICE_NAME);
}


void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  switch(event){
    case ARDUINO_EVENT_WIFI_STA_START:
      WifiState = disconnected;
      Serial.println("Station Mode Started");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      WifiState = connecting;  // IP not yet assigned
      Serial.println("Connected to: " + String(WiFi.SSID()));
      break;     
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Got IP: ");
//      IPAddress ip = IPAddress(WiFi.localIP());
  //    Serial.println(ip);
      Serial.println(WiFi.localIP());
  
      /*
      if (ip[0] == 0) {
        WifiState = disconnected; // invalid IP
      } else {  */
        WifiState = connected;
//      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      WifiState = disconnected;
      LastTimeWifiReconnect = millis(); // do not reconnect immediatelly, wait a few minutes
      Serial.print("WiFi lost connection. Reason: ");
      Serial.println(info.wifi_sta_disconnected.reason);
//      WiFi.setAutoConnect(true);
//      WiFi.setAutoReconnect(true);   
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      WifiState = wps_success;                  
      esp_wifi_wps_disable();
      display.print("\n ");
      display.println(WiFi.SSID());
      display.display();
      Serial.println("WPS Successful, stopping WPS and connecting to: " + String(WiFi.SSID()));
      delay(10);
// https://stackoverflow.com/questions/48024780/esp32-wps-reconnect-on-power-on      
      WiFi.begin();
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      WifiState = wps_failed;
      Serial.println("WPS Failed, rebooting...");
      display.print(" REBOOT");
      display.display();
      // reboot and retry to connect to WiFi, if maybe network is back up and running.
      delay (2500);
      ESP.restart();
      /*
      Serial.println("WPS Failed, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&wps_config);
      esp_wifi_wps_start(0);
      */
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      WifiState = wps_failed;
      Serial.println("WPS Timeout, rebooting...");
      display.print(" REBOOT");
      display.display();
      // reboot and retry to connect to WiFi, if maybe network is back up and running.
      delay (2500);
      ESP.restart();
      /*
      Serial.println("WPS Timeout, retrying");
      display.print("/");  // retry
      display.display();
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&wps_config);
      esp_wifi_wps_start(0);
      WifiState = wps_active;
      */
      break;
    default:
      break;
  }
}



void WiFiStartWps() {
   
  display.clearDisplay();  
  display.setCursor(0, 0);
  display.print("PRESS WPS BUTTON \nON ROUTER!  ");
  display.display();

  Serial.println("Starting WPS");
  
  WifiState = wps_active;
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);

  wpsInitConfig();
  esp_wifi_wps_enable(&wps_config);
  esp_wifi_wps_start(0);  


  // loop until connected
  while (WifiState != connected) {
    delay(2500);
    display.print("*");
    display.display();
    Serial.print("*");
  }
  Serial.println(" WPS finished."); 
  display.clearDisplay();  
  display.setCursor(0, 0);
  display.print("WPS OK!");
}

void WifiBegin()  {
  WifiState = disconnected;

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  
  WiFi.setHostname(DEVICE_NAME);  

    display.print("WiFi");
    display.display();
    Serial.print("Joining wifi ");
    
//    WiFi.setAutoConnect(true);
//    WiFi.setAutoReconnect(true);   
    // https://stackoverflow.com/questions/48024780/esp32-wps-reconnect-on-power-on
    WiFi.begin("HA2", "DomaceNaprave444");  // use internally saved data

    unsigned long StartTime = millis();

    while ((WiFi.status() != WL_CONNECTED)) {
      delay(500);
      display.print(".");
      display.display();
      Serial.print(".");
      if ((millis() - StartTime) > (WIFI_CONNECT_TIMEOUT_SEC * 1000)) {
        Serial.println("\r\nWiFi connection timeout!");
        display.println("TIMEOUT!");
        display.display();
        WifiState = disconnected;
//        return; // exit loop, exit procedure, continue clock startup
//        WiFi.setAutoConnect(false);
//        WiFi.setAutoReconnect(false);   
        WiFiStartWps(); // infinite loop until connected
//        WiFi.setAutoReconnect(true);   
      }
    }
 
  WifiState = connected;

  display.print("\nWiFi: ");
  display.println(WiFi.SSID());
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(400);
}

void CheckWiFiStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - LastTimeWifiReconnect > WIFI_RECONNECT_RETRY_MIN * 60000) {
      LastTimeWifiReconnect = millis();
      Serial.println("Reconnecting to WiFi...");
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.print("Connecting... ");
      WifiState = connecting;
      WiFi.begin();  // use internally saved data
    }
  }
}



// Get an API Key by registering on
// https://ipgeolocation.io
// OR


bool GetGeoLocationTimeZoneOffset() {
  Serial.println("Starting Geolocation query...");
// https://app.abstractapi.com/api/ip-geolocation/    // free for 5k loopkups per month.
// Live test:  https://ipgeolocation.abstractapi.com/v1/?api_key=e11dc0f9bab446bfa9957aad2c4ad064
  IPGeolocation location(GEOLOCATION_API_KEY,"ABSTRACT");
  IPGeo IPG;
  if (location.updateStatus(&IPG)) {
   
    Serial.println(String("Geo Time Zone: ") + String(IPG.tz));
    Serial.println(String("Geo TZ Offset: ") + String(IPG.offset));  // we are interested in this one, type = double
    Serial.println(String("Geo DST: ") + String(IPG.is_dst));  // we are interested in this one, type = boolean
    Serial.println(String("Geo Current Time: ") + String(IPG.current_time)); // currently not used
    GeoLocTZoffset = IPG.offset;  // DST offset is already included in Timezone offset! ignore the "is_dst" value.

    return true;
  } else {
    Serial.println("Geolocation failed.");    
    return false;
  }
}

#include <PubSubClient.h>  // Download and install this library first from: https://www.arduinolibraries.info/libraries/pub-sub-client
#include <WiFiClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

int splitTopic(char* topic, char* tokens[], int tokensNumber);
void callback(char* topic, byte* payload, unsigned int length);
void sendToBroker(char* topic, char* message);
void sendValue();
uint32_t lastTimeSentMQTT = 0;
void MQTTLoop();
void startMqtt();
void checkMqtt();


#include "MHZ_AO.h"
MHZ co2(MH_Z19_RX, MH_Z19_TX, MHZ19C);
uint32_t lastTimeSensorRead = 0;




// ##### WEBSERVER #####

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
//JSONVar readings;

// Timer variables
unsigned long lastTimeSentToWebserver = 0;




// Get Sensor Readings and return JSON object
String PrepareJsonDataForWeb(){
  // Allocate a temporary JsonDocument
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
// JsonDocument<50> readings;
  DynamicJsonDocument readings(50);
  readings["CO2ppmRaw"] = String(co2.CO2ppm);
  readings["CO2ppmFlt"] = String(co2.CO2ppmFiltered);  

//  String jsonString = JSON.stringify(readings);
  String jsonString;
  serializeJson(readings, jsonString);
  return jsonString;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    display.write("SPIFFS ERROR\n");
    display.display();
  }
  else{
    Serial.println("SPIFFS mounted successfully");
    display.write("SPIFFS OK\n");
    display.display();
  }
}


void WebserverBegin() {
  initSPIFFS();

  display.write("Webserver start\n");
  display.display();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client connected");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client requesting data");
    String json = PrepareJsonDataForWeb();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void WebserverLoop() {
  if (((millis() - lastTimeSentToWebserver) > WEBSERVER_SEND_TO_CHARTS * 1000) && (co2.DataValid == coSTATUS_OK) && (WifiState == connected)) {
    // Send Events to the client with the Sensor Readings Every x seconds
    events.send("ping",NULL,millis());
    events.send(PrepareJsonDataForWeb().c_str(),"new_readings" ,millis());
    lastTimeSentToWebserver = millis();
  }
}







void setup() {
  Serial.begin(115200);
  delay(1000);  // Waiting for serial monitor to catch up.
  Serial.println("");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("In setup().");  

  oledBegin();

  WifiBegin();
/*  
  // wait for a bit before querying NTP
  for (uint8_t ndx=0; ndx < 5; ndx++) {
    display.print(".");
    display.display();
    delay(100);
  }
  display.println("");
  */

  // Setup the clock.  It needs WiFi to be established already.
#if (SCREEN_HEIGHT == 64)
  display.println("Clock start...");
#endif
  display.display();
  uclock.begin();

  display.print("NTP = ");
  display.println(uclock.ntpTimeClient.getFormattedTime());
  display.display();

#ifdef GEOLOCATION_ENABLED
  display.print("Geoloc... ");
  display.display();
  if (GetGeoLocationTimeZoneOffset()) {
    display.print("TZ = ");
    display.println(GeoLocTZoffset);
    uclock.setTimeZoneOffset(GeoLocTZoffset * 3600);
    Serial.println(" Done.");
  } else {
    Serial.println("Geolocation failed.");    
    display.println("FAILED");
  }
  display.display();
#endif

  // enable debug to get addition information
  co2.setDebug(CO2_SERIAL_DEBUG);

  startMqtt();

  WebserverBegin();
    
  // Leave boot up messages on screen for a few seconds.
  for (uint8_t ndx=0; ndx < 10; ndx++) {
    display.print(">");
    display.display();
    delay(200);
  }

  // Start up the clock displays.


  uclock.loop();
  LastTimeDisplayChanged = millis();
  updateClockDisplay();
  Serial.println("Setup finished.");
}


// ----------------------------------------------------------------------------------------------------------------------------


void callback(char* topic, byte* payload, unsigned int length) {  //A new message has been received
  Serial.print("Topic:");
  Serial.println(topic);
  int tokensNumber = 10;
  char* tokens[tokensNumber];
  char message[length + 1];
  tokensNumber = splitTopic(topic, tokens, tokensNumber);
  sprintf(message, "%c", (char)payload[0]);
  for (int i = 1; i < length; i++) {
    sprintf(message, "%s%c", message, (char)payload[i]);
  }
  Serial.print("Message:");
  Serial.println(message);

    if (tokensNumber < 3) {
        // otherwise code below crashes on the strmp on non-initialized pointers in tokens[] array
        Serial.println("Number of tokens in MQTT message < 3!");
        return; 
    }

  //------------------ACTIONS HERE---------------------------------

  if (strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "powerState") == 0) {
    if (strcmp(message, "ON") == 0) {
      DisplayOFF = 0;
      Serial.println("Display enabled.");
    } else if (strcmp(message, "OFF") == 0) {
      DisplayOFF = 1;
      Serial.println("Display disabled.");
    }
  }
}


void startMqtt() {
  if (WifiState == connected) {  
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
  
    Serial.print("Connecting to MQTT... ");
    display.write("MQTT start...");
    display.display();
  
    if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      display.write(" OK \n");
      display.display();
  
      char subscibeTopic[100];
      sprintf(subscibeTopic, "%s/#", MQTT_CLIENT);
      client.subscribe(subscibeTopic);  //Subscribes to all messages send to the device
    
      sendToBroker("report/online", "true");  // Reports that the device is online
      delay(100);
      sendToBroker("report/firmware", DEVICE_NAME);  // Reports the firmware version
      delay(100);
      sendToBroker("report/ip", (char*)WiFi.localIP().toString().c_str());  // Reports the ip
      delay(100);
      sendToBroker("report/network", (char*)WiFi.SSID().c_str());  // Reports the network name
      delay(100);
      ReportBack(); // all other MQTT data
    
      char signal[5];
      sprintf(signal, "%d", WiFi.RSSI());
      sendToBroker("report/signal", signal);  // Reports the signal strength
      delay(100);
      
    } else {
       Serial.println("");
      if (client.state() == 5) {
        Serial.println("Connection not allowed by broker, possible reasons:");
        Serial.println("- Device is already online. Wait some seconds until it appears offline for the broker");
        Serial.println("- Wrong Username or password. Check credentials");
        Serial.println("- Client Id does not belong to this username, verify ClientId");
        display.write(" ERR 5\n");
        display.display();
  
      } else {
        Serial.print("Not possible to connect to Broker; Error code: ");
        Serial.println(client.state());
        display.write(" ERR X\n");
        display.display();
      }
  
       delay(0x7537);
    }  // not allowed to connect to broker
  }  // wifi connected
}


int splitTopic(char* topic, char* tokens[], int tokensNumber) {
    const char s[2] = "/";
    int pos = 0;
    tokens[0] = strtok(topic, s);
    while (pos < tokensNumber - 1 && tokens[pos] != NULL) {
        pos++;
        tokens[pos] = strtok(NULL, s);
    }
    return pos;
}

void checkMqtt() {
  if ((WifiState == connected) && (WiFi.status() == WL_CONNECTED)) {
    if (!client.connected()) {
      Serial.println("Not connected to MQTT server, reconnecting...");
      startMqtt();
      }
    }
  }

void sendToBroker(char* topic, char* message) {
  if (client.connected()) {
    char topicArr[100];
    sprintf(topicArr, "%s/%s", MQTT_CLIENT, topic);
    client.publish(topicArr, message);
    delay(100);
  }
}


void ReportBack() {
  if ((millis() - lastTimeSentMQTT > MQTT_STATUS_REPORT_SEC * 1000) && client.connected() && (WifiState == connected)) {
    if (coSTATUS_OK == co2.DataValid) {
      Serial.println("Sending MQTT data...");
      char message[15];      
      sprintf(message, "%d", co2.Temperature);
      sendToBroker("report/temperature", message);
  /*
  voltage_temp=(adcReadValue*3300+512)/1024; // Voltage in mV. +512 for correct rounding.
  sprintf(str,"%d.%03d",voltage_temp/1000,voltage_temp%1000);
  */      
        int MQTTReportHumidity = (co2.CO2ppmFiltered - 400) / 16;  // 400 ppm = 0% ........ 2000 ppm = 100%
        if (MQTTReportHumidity > 100) MQTTReportHumidity = 100;
        sprintf(message, "%d", MQTTReportHumidity);
        sendToBroker("report/humidity", message);
  
        sprintf(message, "CO2:  %d ppm", co2.CO2ppmFiltered);
        sendToBroker("report/firmware", message);
  //    sendToBroker("report/info", message);
  //    sprintf(message, "%d", BattLevel);
  //    sendToBroker("report/battery", message);

        if (DisplayOFF != 0) {
            sendToBroker("report/powerState", "OFF");
          } else {
            sendToBroker("report/powerState", "ON");
          }
        }
    lastTimeSentMQTT = millis();
  }
}


void MQTTLoop() {
  client.loop();
  checkMqtt();
  ReportBack();
}




uint8_t hour_old = 99;
uint8_t second_old = 99;

// utility function for digital clock display: prints leading 0
String twoDigits(int digits) {
  if (digits < 10) {
    String i = '0' + String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}


void EveryFullHour() {
  // dim the clock at night
  uint8_t current_hour = uclock.getHour24();
  if (current_hour != hour_old) {  // FullHour
  Serial.print("current hour = ");
  Serial.println(current_hour);
    if ((current_hour >= NIGHT_TIME) || (current_hour < DAY_TIME)) {
      Serial.println("Setting night mode (dimmed)");
      display.dim(true);
      //display.setContrast(1);
    } else {
      Serial.println("Setting daytime mode (normal brightness)");
      display.dim(false);
    }
    
    if (uclock.getHour24() == 4) // Daylight savings time changes at 3 in the morning -> change to 4 not to interfere with the other clock
      if (GetGeoLocationTimeZoneOffset())       // run once a day (= 744 times per month which is below the limit of 5k for free account)
          uclock.setTimeZoneOffset(GeoLocTZoffset * 3600);
    
    hour_old = current_hour;
  }   
}


void ReadCo2Sensor() {
    co2.ReadUART();

    Serial.print("PPMuart: ");
    if (co2.DataValid == coSTATUS_OK) {
      Serial.print(co2.CO2ppm);
    } else {
      Serial.print("n/a");
    }
  
    Serial.print(", Temperature: ");
  
    if (co2.DataValid == coSTATUS_OK) {
      Serial.println(co2.Temperature);
    } else {
      Serial.println("n/a");
    }

    if (co2.DataValid == coSTATUS_OK)
      SensorTxt = co2.CO2ppmFiltered; // co2.CO2ppm;
    else  
      SensorTxt = "N/A";
}


#if (SCREEN_HEIGHT == 32)
  #include <Fonts/FreeSerifBold18pt7b.h>
#else  
  #include <Fonts/FreeSansBold24pt7b.h>
#endif

void DisplayCo2(int Xoffset) {
  #if (SCREEN_HEIGHT == 32)
    display.setFont(&FreeSerifBold18pt7b); 
    byte y1 = 29;
    byte y2 = SCREEN_HEIGHT;
  #else 
    display.setFont(&FreeSansBold24pt7b); 
    byte y1 = 45;
    byte y2 = 45;
  #endif

    int16_t  x0, y0;
    uint16_t w, h; 
    display.getTextBounds(SensorTxt, 0, 0, &x0, &y0, &w, &h);
//    display.setCursor(SCREEN_WIDTH-18-w-8+Xoffset, 12 + h);
    display.setCursor(SCREEN_WIDTH-18-w-8+Xoffset, y1);
    
//    display.setCursor(5+Xoffset, y1);
    display.print(SensorTxt);

    display.setFont();           // default built in 6x8 font
    display.setTextSize(1);
    if (Xoffset <= 0) {
      display.setCursor(SCREEN_WIDTH-18+Xoffset, y2-8);
      display.write("PPM");
    }

  #if (SCREEN_HEIGHT == 64)  
/*
    display.setFont();           // default built-in font
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(2, 0);     // Start at top-left corner
    display.write("CO2:");
    // raw value
    sprintf(txt, "%d", co2.CO2ppm); 
    display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH-w-2, 0);     // top-right corner
    display.write(txt);
*/  
    display.setCursor(65+Xoffset, SCREEN_HEIGHT-8);
    display.print("Temp: " + String(co2.Temperature) + " C");
  #endif
}


void DisplayClock(int Xoffset) {
  #if (SCREEN_HEIGHT == 32)
    display.setFont(&FreeSerifBold18pt7b); 
    byte y = 29;
    byte x = 15;
  #else 
    display.setFont(&FreeSansBold24pt7b); 
    byte y = 45;
    byte x = 5;
  #endif

    display.setCursor(x+Xoffset, y);
    display.print(ClockTxt);
  
    // sekunde v desnem spodnjem kotu
    uclock.loop();   // during shifting seconds must be updated
    display.setFont();           // default built in 6x8 font
    display.setTextSize(1);
    display.setCursor(SCREEN_WIDTH-14+Xoffset, SCREEN_HEIGHT-8); // text anchor: top left
    display.print(twoDigits(uclock.getSecond()));
}


int shift;

void updateClockDisplay() {
  if (DisplayOFF == 0) {
    if (CurrentDisplayClock) {
      uint8_t current_second = uclock.getSecond();
      if (current_second != second_old) {  // redraw only if time has changed
        ClockTxt = twoDigits(uclock.getHour24()) + ':' + twoDigits(uclock.getMinute());
  
        display.clearDisplay();
        DisplayClock(0);
  
        display.setCursor(0, SCREEN_HEIGHT-8);
        if (WifiState == disconnected) display.print("No WiFi ");
        if (WifiState == connecting) display.print("Connecting... ");
        if (!uclock.time_valid) display.print("Unsync");
   
        display.display();
      }  // update every sec
  
      if (millis() - LastTimeDisplayChanged > SHOW_TIME_SEC * 1000) {
        CurrentDisplayClock = !CurrentDisplayClock;
        LastTimeDisplayChanged = millis();
        // update CO2 
        ReadCo2Sensor();
        // shift displays
        for (shift=0; shift > -SCREEN_WIDTH-1; shift -= SCROLL_PIXELS) {
          display.clearDisplay();  
          DisplayClock(shift);
          DisplayCo2(shift + SCREEN_WIDTH);
          display.display();
          delay (SCROLL_DELAY);
        }  // for      
      }  // shift
    } // show clock
    else {
      if ((millis()-lastTimeSensorRead) > (READ_CO2_SENSOR_SEC * 1111+11)) {
        lastTimeSensorRead = millis();
  
        ReadCo2Sensor();
  
        display.clearDisplay();
        DisplayCo2(0); 
        display.display();
      }  // update every x seconds
  
      if (millis() - LastTimeDisplayChanged > SHOW_SENSOR_SEC * 1000) {
        CurrentDisplayClock = !CurrentDisplayClock;
        LastTimeDisplayChanged = millis();
        // update Clock
        ClockTxt = twoDigits(uclock.getHour24()) + ':' + twoDigits(uclock.getMinute());
        // shift displays
        for (shift=0; shift > -SCREEN_WIDTH-1; shift -= SCROLL_PIXELS) {
          display.clearDisplay();  
          DisplayCo2(shift);
          DisplayClock(shift + SCREEN_WIDTH);
          display.display();
          delay (SCROLL_DELAY);
        }  // for      
      }  // refresh sensor
    }  // show sensor
  } else // DisplayOFF != 0
   { 
    if (DisplayOFF == 1) { // clear the display only on request, not all the time
      Serial.println("Request to turn off the display.");
      display.clearDisplay();
      display.display();
      DisplayOFF = 2;
    }
    if ((millis()-lastTimeSensorRead) > (READ_CO2_SENSOR_SEC * 1111+11)) {
      lastTimeSensorRead = millis();
      ReadCo2Sensor();
    }
   } // display off
}



void loop() {
  uint32_t millis_at_top = millis();
  uclock.loop();
  EveryFullHour(); // night or daytime
  updateClockDisplay();
  CheckWiFiStatus();
  MQTTLoop();

  WebserverLoop();
  
  uint32_t time_in_loop = millis() - millis_at_top;
  if (time_in_loop < 20) {
      // we have free time
      
      // Sleep for up to 20ms, less if we've spent time doing stuff above.
      time_in_loop = millis() - millis_at_top;
      if (time_in_loop < 20) {
        delay(20 - time_in_loop);
      }
    }
#ifdef DEBUG_OUTPUT
  if (time_in_loop <= 5) Serial.print(".");
  else Serial.println(time_in_loop);
#endif
}
