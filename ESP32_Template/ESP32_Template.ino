//Code for ESP32 board
//=======================Library============================
#include <ThingsBoard.h>    // ThingsBoard SDK
#include <WiFi.h>           // WiFi control for ESP32
#include "DHT.h"

//=======================DEFINE==============================
//WiFi
#define WIFI_AP             "Mimosa"     //WiFi access point name (SSID)
#define WIFI_PASSWORD       "Mimosa1"   //WiFi password
//Thingsboard
#define TOKEN               "qj1qvMZZDD8yu9KeAF8l"  //Access token
#define THINGSBOARD_SERVER  "192.168.0.221"         //ThingsBoard Server IP
// Sensors
#define DHTPIN 4          // DHT22 humidity & temperature sensor on pin 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//===================Global Variables========================
float dht_data[] = {0.0,0.0};         // To store value of temperature and humidity in an array

//=========================MAIN==============================
WiFiClient espClient;       // Initialize ThingsBoard client
ThingsBoard tb(espClient);  // Initialize ThingsBoard instance
int status = WL_IDLE_STATUS;// the Wifi radio's status

void setup() {
  Serial.begin(115200);     // Baud rate for debug serial
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  InitWiFi();
  dht.begin();
}

void loop() {
  // Get data from sensor and update variable
  update_DHT22();         // Update sensor data to dht_data[] array declared above
  print_serial_DHT22();   // Print sensor output from dht_data[] to serial monitor
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }
  
  // Check if connected to ThingsBoard
  if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }
  
  // Send data to ThingsBoard
  Serial.println("Sending data...");
  tb.sendTelemetryFloat("Temperature", dht_data[0]);  //Send temperature data to ThingsBoard
  tb.sendTelemetryFloat("Humidity", dht_data[1]);     //Send humidity data to ThingsBoard

  tb.loop();
  //delay(3000);  // Update interval
}

//=====================Input Functions=======================
void update_DHT22(){     //DHT22 Temperature and Humidity Sensor
  delay(2000);
  dht_data[0] = dht.readTemperature();    //store temperature in index 0
  dht_data[1] = dht.readHumidity();       //store humidity in index 1
}

//=====================Misc. Functions=======================
void print_serial_DHT22(){ // Print serial data of DHT22 sensor to serial monitor
  Serial.print("DHT22 sensor [");
  Serial.print("Temperature: ");
  Serial.print(dht_data[0]);
  Serial.print(" °C | Humidity: ");
  Serial.print(dht_data[1]);
  Serial.println(" %RH]");
}

//=====================Wi-Fi Functions=======================
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}
