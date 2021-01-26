//Code for ESP32 board
//=======================Library============================
#include "DHT.h"

//=======================DEFINE==============================
// Sensors
#define DHTPIN 4          // DHT22 humidity & temperature sensor on pin 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//===================Global Variables========================
float dht_data[] = {0.0,0.0};         // To store value of temperature and humidity in an array

//=========================MAIN==============================
void setup() {
  // initialize serial for debugging
  Serial.begin(115200);// Baud rate for debug serial
  dht.begin();
}

void loop() {
  update_DHT22();
  print_serial_DHT22();
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
