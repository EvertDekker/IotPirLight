/*
    Gerneric Esp8266 Module: Flash Mode: QIO, Flash Frequency: 40MHz, CPU Frequency: 80MHz
    Flash Size: 4M (1M SPIFFS), Debug Port: Disabled, Debug Level: None,  Reset Method: ck
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MQTTClient.h>

const char* vrs = "1.0"; // Evert Dekker 2017
const char* ssid = "...";
const char* password = "123";
const char* hostn = "IotPirLight";

IPAddress ip(192,168,1,10);
IPAddress gateway(192,168,1,254);
IPAddress subnet(255,255,255,0);

//Set the debug output, only one at the time
#define NONE true
//#define SERIAL true

#ifdef  NONE
#define DEBUG_PRINTF(x,...)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#elif SERIAL
#define DEBUG_PRINTF(x,...) Serial.printf(x, ##__VA_ARGS__)
#define DEBUG_PRINTLN(x)    Serial.println(x)
#define DEBUG_PRINT(x)      Serial.print(x)
#endif


// Config

// declaration
void connect(); // <- predefine connect() for setup Mqtt()

//WiFiClientSecure net ;
WiFiClient net;
MQTTClient client;

// globals
unsigned long startMills;
int nodestatus;
unsigned long lastMillis = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostn);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);


  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_PRINTLN("Connection Failed! Rebooting...");
    ESP.deepSleep(120 * 1000000);   // deepSleep time is defined in microseconds. Multiply seconds by 1e6
    delay(1000);
  }
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(hostn);

  // No authentication by default
 // ArduinoOTA.setPassword((const char *)OTA_PASSW);

  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });

  ArduinoOTA.begin();
  DEBUG_PRINTLN("Ready");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT("Hostname: ");
  DEBUG_PRINTLN(WiFi.hostname());
  //client.begin("192.168.1.127", 8883, net); // MQTT brokers usually use port 8883 for secure connections
  client.begin("192.168.1.111", net);
  client.onMessage(messageReceived);
  connect();
}

void loop() {
  ArduinoOTA.handle();

  delay(10); // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    DEBUG_PRINTLN("\Retry connecting Mqtt");
    connect();
  }
   client.loop();

  if (nodestatus == 15) {  //You can change status with mqtt broker to not go in deep sleep
    DEBUG_PRINT("Not going to sleep");
    client.publish("/vm/tuin/pirsensorachter/status", String(nodestatus), true, 0);
    for (int i = 1; i < 30; i++) { //waiting 30 seconds for OTA
      DEBUG_PRINTLN("Waiting for OTA");
      ArduinoOTA.handle();
    }
    DEBUG_PRINTLN("No OTA received, back to normal");
    nodestatus = 0;
  }
  else
  {
    client.publish("/vm/tuin/pirsensorachter/nodeid", String("123"), true, 0);
    client.publish("/vm/tuin/pirsensorachter/versie", String(vrs), true, 0);
    client.publish("/vm/tuin/pirsensorachter/alarm", "1" , true, 0);
    client.publish("/vm/tuin/pirsensorachter/status", String(nodestatus), true, 0);
   
    client.loop();
    ESP.deepSleep(300 * 1000000);   // deepSleep time is defined in microseconds. Multiply seconds by 1e6
    delay(1000);
  }
}




void connect() {
  DEBUG_PRINT("Mqtt checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(".");
    delay(1000);
  }

  DEBUG_PRINT("\nMqtt connecting...");
  while (!client.connect(hostn, "MyMqttLogin" , "MyMqttPassword")) {
    DEBUG_PRINT(".");
    delay(1000);
  }
  DEBUG_PRINTLN("\nMqtt connected!");
  client.subscribe("/vm/tuin/pirsensorachter/status");
}


void messageReceived(String &topic, String &payload) {
  nodestatus = payload.toInt();
  DEBUG_PRINT("incoming: ");
  DEBUG_PRINT(topic);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(payload);
  DEBUG_PRINTLN();
}



