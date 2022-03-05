#include "WiFi.h"

#define DEVICE_NAME       "CO2-sensor"


// ##### CO2 #####

#include "MHZ_AO.h"

/*
* Tested with sensors:
* https://www.aliexpress.com/item/1005002994757073.html
* https://www.aliexpress.com/item/1005001947070873.html
*/
// pin for ESP32 uart2 reading
#define MH_Z19_RX 25
#define MH_Z19_TX 26
#define READ_SENSORS_SEC  5

MHZ co2(MH_Z19_RX, MH_Z19_TX, MHZ19C);

uint32_t lastTimeSensorRead = 0;
void  ReadCo2SensorAndDisplay(void);


// ##### MQTT #####

#include <PubSubClient.h>  // Download and install this library first from: https://www.arduinolibraries.info/libraries/pub-sub-client
#include <WiFiClient.h>

#define SSID_NAME "YOUR_WIFI_NETWORK"                         // Your Wifi Network name
#define SSID_PASSWORD "YOUR_PASSWORD"                 // Your Wifi network password
#define MQTT_BROKER "smartnest.cz"                    // Broker host
#define MQTT_PORT 1883                                // Broker port
#define MQTT_USERNAME "YOUR_USERNAME"                      // Username from Smartnest
#define MQTT_PASSWORD "YOUR_API_KEY"                      // Password from Smartnest (or API key)
#define MQTT_CLIENT "YOUR_DEVICE_ID"                       // Device Id from smartnest
#define FIRMWARE_VERSION "CO2 v0.1"  // Custom name for this program
#define STATUS_TX_SEC 15

WiFiClient espClient;
PubSubClient client(espClient);

int splitTopic(char* topic, char* tokens[], int tokensNumber);
void callback(char* topic, byte* payload, unsigned int length);
void sendToBroker(char* topic, char* message);
void turnOff();
void turnOn();
void sendValue();
uint32_t lastTimeSentMQTT = 0;
void MQTTLoop();
void startWifi();
void startMqtt();
void checkMqtt();


// ##### OLED #####

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Black board
// https://www.aliexpress.com/item/4000065217965.html
// I2C Communication SDA = 5 and SCL = 4 on Wemos Lolin32 ESP32 with built-in SSD1306 OLED
#define DISPLAY_SDA 5
#define DISPLAY_SCL 4
/*  
// White board
// https://www.aliexpress.com/item/32847022581.html
#define DISPLAY_SDA 4
#define DISPLAY_SCL 15
*/

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);



// ##### WEBSERVER #####

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#define WEBSERVER_SEND_TO_CHARTS  30

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTimeSentToWebserver = 0;







void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("MHZ 19C");

  // Start I2C Communication with OLED Display
  Wire.begin(DISPLAY_SDA, DISPLAY_SCL);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
 
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(2, 1);     // Start at top-left corner
  display.write("Init...\n");
  display.display();
  
 

  // enable debug to get addition information
  co2.setDebug(true);
/*
  if (co2.isPreHeating()) {
    Serial.print("Preheating");
    while (co2.isPreHeating()) {
      Serial.print(".");
      delay(5000);
    }
    Serial.println();
  }
  */

  startWifi();
  startMqtt();
  WebserverBegin();
  delay (1500);
}

void loop() {
  ReadCo2SensorAndDisplay();
  MQTTLoop();
  WebserverLoop();
  delay (50); // do something else too
}






void ReadCo2SensorAndDisplay() {
  
  if ((millis()-lastTimeSensorRead) > READ_SENSORS_SEC * 1000) {
    Serial.print("\n----- Time from start: ");
    Serial.print(millis() / 1000);
    Serial.println(" s");

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
 
    Serial.println("\n------------------------------");
    char txt[10];
  
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
  
    display.setFont();           // default built-in font
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(2, 1);     // Start at top-left corner
    display.write("CO2:");
  
  //  display.setTextSize(4);
  //  display.setCursor(5, 15);
    display.setFont(&FreeSansBold18pt7b);
    if (co2.DataValid == coSTATUS_OK)
      sprintf(txt, "%d", co2.CO2ppm); 
    else  
      sprintf(txt, "N/A"); 
  
    int16_t  x1, y1;
    uint16_t w, h; 
    display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  
    display.setCursor(SCREEN_WIDTH-25-w-8, 15 + h);
    display.write(txt);
  
    display.setFont();           // default built-in font
    display.setTextSize(1);
    display.setCursor(SCREEN_WIDTH-25, 15+h-8);
    display.write("PPM");
  
    display.setCursor(40, SCREEN_HEIGHT-8);
    display.write("Temp: ");
    sprintf(txt, "%d", co2.Temperature); 
  
    display.write(txt);
    display.write(" C");  
    
    display.display();

    lastTimeSensorRead = millis();
  }
}











void callback(char* topic, byte* payload, unsigned int length) {  //A new message has been received
  Serial.print("Topic:");
  Serial.println(topic);
  int tokensNumber = 10;
  char* tokens[tokensNumber];
  char message[length + 1];
  splitTopic(topic, tokens, tokensNumber);
  sprintf(message, "%c", (char)payload[0]);
  for (int i = 1; i < length; i++) {
    sprintf(message, "%s%c", message, (char)payload[i]);
  }
  Serial.print("Message:");
  Serial.println(message);

  //------------------ACTIONS HERE---------------------------------

  if (strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "powerState") == 0) {
    if (strcmp(message, "ON") == 0) {
      turnOn();
    } else if (strcmp(message, "OFF") == 0) {
      turnOff();
    }
  }
}

void startWifi() {
  display.write("WiFi start");
  display.display();
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  
  WiFi.setHostname(DEVICE_NAME); 
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  Serial.println("Connecting ...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    attempts++;
    delay(500);
    Serial.print(".");
    display.write(".");
    display.display();
  }

  Serial.println('\n');
  display.write('\n');

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    display.write("SSID: ");
    display.write((char*)WiFi.SSID().c_str());
    display.write("\nIP: ");
    display.write((char*)WiFi.localIP().toString().c_str());
    display.write("\n");

  } else {
    Serial.println('I could not connect to the wifi network!\n');
    display.write("ERR\n");
  }

  display.display();
  delay(500);
}

void startMqtt() {
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    display.write("MQTT start...");
    display.display();

    if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      display.write(" OK \n");
      display.display();
    } else {
      if (client.state() == 5) {
        Serial.println("Connection not allowed by broker, possible reasons:");
        Serial.println("- Device is already online. Wait some seconds until it appears offline for the broker");
        Serial.println("- Wrong Username or password. Check credentials");
        Serial.println("- Client Id does not belong to this username, verify ClientId");
        display.write(" ERR 5\n");
        display.display();

      } else {
        Serial.println("Not possible to connect to Broker Error code: ");
        Serial.print(client.state());
        display.write(" ERR X\n");
        display.display();
      }

      delay(0x7530);
    }
  }

  char subscibeTopic[100];
  sprintf(subscibeTopic, "%s/#", MQTT_CLIENT);
  client.subscribe(subscibeTopic);  //Subscribes to all messages send to the device

  sendToBroker("report/online", "true");  // Reports that the device is online
  delay(100);
  sendToBroker("report/firmware", FIRMWARE_VERSION);  // Reports the firmware version
  delay(100);
  sendToBroker("report/ip", (char*)WiFi.localIP().toString().c_str());  // Reports the ip
  delay(100);
  sendToBroker("report/network", (char*)WiFi.SSID().c_str());  // Reports the network name
  delay(100);

  char signal[5];
  sprintf(signal, "%d", WiFi.RSSI());
  sendToBroker("report/signal", signal);  // Reports the signal strength
  delay(100);
}

void MQTTLoop() {
  client.loop();
  checkMqtt();
  ReportBack();
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
  if (!client.connected()) {
    Serial.printf("Device lost connection with broker status %, reconnecting...\n", client.connected());

    if (WiFi.status() != WL_CONNECTED) {
      startWifi();
    }
    startMqtt();
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

void turnOff() {
  Serial.println("Turning off...\n");
  sendToBroker("report/powerState", "OFF");
}

void turnOn() {
  Serial.println("Turning on...\n");
  sendToBroker("report/powerState", "ON");
}


void ReportBack() {
  if ((millis() - lastTimeSentMQTT > STATUS_TX_SEC * 1000) && client.connected()) {
    if (coSTATUS_OK == co2.DataValid) {
      Serial.println("Sending MQTT data...");
      char message[15];      
      sprintf(message, "%d", co2.Temperature);
      sendToBroker("report/temperature", message);
  /*
  voltage_temp=(adcReadValue*3300+512)/1024; // Voltage in mV. +512 for correct rounding.
  sprintf(str,"%d.%03d",voltage_temp/1000,voltage_temp%1000);
  */      
        int MQTTReportHumidity = (co2.CO2ppm - 400) / 16;  // 400 ppm = 0% ........ 2000 ppm = 100%
        if (MQTTReportHumidity > 100) MQTTReportHumidity = 100;
        sprintf(message, "%d", MQTTReportHumidity);
        sendToBroker("report/humidity", message);
  
        sprintf(message, "CO2:  %d ppm", co2.CO2ppm);
        sendToBroker("report/firmware", message);
  //      sendToBroker("report/info", message);
  //    sprintf(message, "%d", BattLevel);
  //    sendToBroker("report/battery", message);
        }
    lastTimeSentMQTT = millis();
  }
}











// Get Sensor Readings and return JSON object
String PrepareJsonDataForWeb(){
  readings["CO2ppm"] = String(co2.CO2ppm);

  String jsonString = JSON.stringify(readings);
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
  if (((millis() - lastTimeSentToWebserver) > WEBSERVER_SEND_TO_CHARTS * 1000) && (co2.DataValid == coSTATUS_OK)) {
    // Send Events to the client with the Sensor Readings Every x seconds
    events.send("ping",NULL,millis());
    events.send(PrepareJsonDataForWeb().c_str(),"new_readings" ,millis());
    lastTimeSentToWebserver = millis();
  }
}
