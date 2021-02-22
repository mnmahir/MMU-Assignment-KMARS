//Code for ESP32 board
//=======================Library============================
#include <ThingsBoard.h>    // ThingsBoard SDK
#include <analogWrite.h>
#include <WiFi.h>           // WiFi control for ESP32
#include "DHT.h"
#include <SPI.h>            // OLED
#include <Adafruit_GFX.h>   // OLED
#include <Adafruit_SSD1306.h>// OLED
//#include <LiquidCrystal_I2C.h> // LCD library using from  https://www.ardumotive.com/i2clcden.html for the i2c LCD library 

//=======================CONSTANTS==============================
//WiFi
#define WIFI_AP             "musyu99"     //WiFi access point name (SSID)
#define WIFI_PASSWORD       "2233musyu"   //WiFi password

//Thingsboard
#define TOKEN               "nCtnZVZkyZ6vqQ5n4rth"  //Access token
#define THINGSBOARD_SERVER  "192.168.0.154"         //ThingsBoard Server IP

// Inputs
#define DHTPIN 4          // DHT22 humidity & temperature sensor on pin 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#define MQ2PIN 36
#define MQ5PIN 34  
#define FlamePIN 32

// Outputs
#define BuzzPIN 25
#define WaterPumpPIN 16
#define FanPIN 17

// OLED SDA -> 21 & SCL -> 22
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//===================Global Variables========================
// For sensors
float dht_data[] = {0.0,0.0};         // To store value of temperature and humidity in an array
int mq5_data = 0.0;
int mq2_data = 0.0;
int flame_data = 0.0;

// For display
int alarm_level = 0; // 0 = No Issues, 1 = Need attention, 2 = DANGER!
int display_page = 0; // 0 = Temperature and humidity, 1 = Gas/Smoke level, 3 = Warning!





//=========================MAIN==============================
WiFiClient espClient;       // Initialize ThingsBoard client
ThingsBoard tb(espClient);  // Initialize ThingsBoard instance
int status = WL_IDLE_STATUS;// the Wifi radio's status

void setup() {
  ledcAttachPin(BuzzPIN, 0);  //Attach Buzzer Channel 0
  Serial.begin(115200);     // Baud rate for debug serial
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  Serial.println("Initializing...");
  InitWiFi();
  dht.begin();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

void loop() {
  check_wifi();
  check_tb_connection();
  // Get data from sensor and update variable
  update_DHT22();         // Update sensor data to dht_data[] float array declared above
  update_MQ2();     // Update sensor data to mq2_data integer variable
  update_MQ5();
  update_Flame();
  print_serial();   // Print to serial monitor
  print_oled_display();
  buzz_alarm();
  
  //delay(1000);
  // Send data to ThingsBoard
  Serial.println("Sending data...");
  tb.sendTelemetryFloat("Temperature", dht_data[0]);  //Send temperature data to ThingsBoard
  tb.sendTelemetryFloat("Humidity", dht_data[1]);     //Send humidity data to ThingsBoard
  tb.sendTelemetryFloat("MQ2", mq2_data);     //Send MQ2 data to ThingsBoard
  tb.sendTelemetryFloat("MQ5", mq5_data);     //Send MQ5 data to ThingsBoard
  tb.sendTelemetryFloat("Flame", flame_data);     //Send Flame Sensor data to ThingsBoard

  tb.loop();
  //delay(3000);  // Update interval
}













//=====================Input Functions (Sensors)=======================
void update_DHT22(){     //DHT22 Temperature and Humidity Sensor
  delay(2000);
  dht_data[0] = dht.readTemperature();    //store temperature in index 0
  dht_data[1] = dht.readHumidity();       //store humidity in index 1
}

void update_MQ2(){     //MQ2
  mq2_data = analogRead(MQ2PIN);
}

void update_MQ5(){     //MQ5
  mq5_data = analogRead(MQ5PIN);
}

void update_Flame(){     //Flame
  flame_data = analogRead(FlamePIN);
}


//=====================Display Functions=======================
// Display to OLED SSD1306
void print_oled_display(void) {
  String temperature = String(dht_data[0]);
  String humidity = String(dht_data[1]);
  String smoke = String(mq2_data);
  String gas = String(mq5_data);
  String flame = String(flame_data);
  display.clearDisplay();      // Clear the buffer
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  if(display_page == 0){
    // display temperature
    display.print("T: ");
    display.print(temperature);
    display.print(" ");
    display.setTextSize(1);
    display.cp437(true);
    display.write(248);
    display.setTextSize(2);
    display.print("C");
    // display humidity
    display.setTextSize(2);
    display.setCursor(0, 25);
    display.print("H: ");
    display.print(humidity);
    display.print(" %"); 
    display_page = 1;
  }
  else if(display_page == 1){
    // display mq2
    display.print("Smoke:");
    display.print(smoke);
    // display mq5
    display.setCursor(0, 25);
    display.print("Gas:");
    display.print(gas);
    // display flame
    display.setCursor(0, 50);
    display.print("Flame:");
    display.print(flame);
    display_page = 0;
  }
  else{
    display.println("Mahir");
    display_page = 0;
  }
  display.display();
}

//Display to serial monitor
void print_serial(){
  Serial.print("DHT22 sensor [");
  Serial.print("Temperature: ");
  Serial.print(dht_data[0]);
  Serial.print(" °C | Humidity: ");
  Serial.print(dht_data[1]);
  Serial.println(" %RH]");
  Serial.print("MQ2 sensor: ");
  Serial.println(mq2_data);
  Serial.print("MQ5 sensor: ");
  Serial.println(mq5_data);
  Serial.print("Flame sensor: ");
  Serial.println(flame_data);
}

//=====================Alarm Functions=======================
void buzz_alarm(){      // Alarm sound
   for(int i = 2000; i<=5000; i+=50){
     ledcWriteTone(0, i);
     delay(10);
   }
   for(int i = 0; i<=25; i++){
     ledcWriteTone(0, 4000);
     delay(25);
     ledcWriteTone(0, 0);
     delay(25);
   }
}


//=====================Wi-Fi Functions (Don't look! scary!) =======================
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

void check_wifi(){
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }  
}
void check_tb_connection(){
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
}
