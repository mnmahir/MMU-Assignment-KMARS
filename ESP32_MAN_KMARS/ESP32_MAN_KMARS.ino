//Code for ESP32 board
//======================= LIBRARY ============================
#include <ThingsBoard.h>    // ThingsBoard SDK
#include <analogWrite.h>
#include <WiFi.h>           // WiFi control for ESP32
#include "DHT.h"
#include <SPI.h>            // OLED
#include <Adafruit_GFX.h>   // OLED
#include <Adafruit_SSD1306.h>// OLED
//#include <LiquidCrystal_I2C.h> // LCD library using from  https://www.ardumotive.com/i2clcden.html for the i2c LCD library 

//======================= CONSTANTS ==============================
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
#define BuzzCH 9    // PWM channel
#define WaterPumpPIN 16
#define FanPIN 17
#define FanCH 5

// OLED SDA -> 21 & SCL -> 22
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//=================== GLOBAL VARIABLES ========================
// For sensors
float dht_data[] = {0.0,0.0};         // To store value of temperature and humidity in an array
int dht_level[] = {1,1};        // 0 = NIL, 1 = Low, 2 = Medium, 3  = High
int dht_level_th0[] = {-40,40,50};        
int dht_level_th1[] = {100,30,20};        
int mq2_data = 0;
int mq2_level = 1;    // 0 = NIL, 1 = Low, 2 = Medium, 3  = High
int mq2_level_th[] = {0,75,150};
int mq5_data = 0;
int mq5_level = 1;    /// 0 = NIL, 1 = Low, 2 = Medium, 3  = High
int mq5_level_th[] = {0,300,500};
int flame_data = 0;
int flame_level = 1;  // 0 = NIL, 1 = Low, 2 = Medium, 3  = High
int flame_level_th[] = {4095,1300,1000};

// For Fan and Pump
float fan_speed = 0;
int water_pump_speed = 0;

// For display & alarm
char alarm_str[4][5] = {"NIL","LOW", "MED", "HIGH"};
int alarm_level = 0;



//=================== FreeRTOS =============================
TaskHandle_t Task_Sensor, Task_Alarm, Task_Display, Task_Fan, Task_Water;



//======================= MAIN ==============================
WiFiClient espClient;       // Initialize ThingsBoard client
ThingsBoard tb(espClient);  // Initialize ThingsBoard instance
int status = WL_IDLE_STATUS;// the Wifi radio's status

void setup() {
  ledcAttachPin(BuzzPIN, BuzzCH);  //Attach Buzzer PWM
  ledcAttachPin(FanPIN, FanCH);  //Attach Fan PWM
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
  print_oled_init();
  check_wifi();
  check_tb_connection();
  delay(3000);
  xTaskCreatePinnedToCore(update_sensor,"sensorTask",10000,NULL,1,&Task_Sensor,1);
  delay(50);  // needed to start-up task
  xTaskCreatePinnedToCore(buzz_alarm,"alarmTask",1000,NULL,1,&Task_Alarm,0);
  delay(50);  // needed to start-up task
  xTaskCreatePinnedToCore(ex_fan,"fanTask",1000,NULL,2,&Task_Fan,0);
  delay(50);  // needed to start-up task
  xTaskCreatePinnedToCore(water_pump,"waterTask",1000,NULL,1,&Task_Water,0);
  delay(50);  // needed to start-up task
  xTaskCreatePinnedToCore(print_oled_display,"displayTask",10000,NULL,5,&Task_Display,1);
  delay(50);  // needed to start-up task
}

void loop() {
  vTaskDelay(2000);
  check_wifi();
  check_tb_connection();
  Serial.println("Sending data...");
  tb.sendTelemetryFloat("Temperature", dht_data[0]);  //Send temperature data to ThingsBoard
  tb.sendTelemetryFloat("Humidity", dht_data[1]);     //Send humidity data to ThingsBoard
  tb.sendTelemetryFloat("MQ2", mq2_data);     //Send MQ2 data to ThingsBoard
  tb.sendTelemetryFloat("MQ5", mq5_data);     //Send MQ5 data to ThingsBoard
  tb.sendTelemetryFloat("Flame", flame_data);     //Send Flame Sensor data to ThingsBoard
  tb.sendTelemetryInt("Temperature_level", dht_level[0]);  //Send temperature data to ThingsBoard
  tb.sendTelemetryInt("Humidity_level", dht_level[1]);     //Send humidity data to ThingsBoard
  tb.sendTelemetryInt("MQ2_level", mq2_level);     //Send MQ2 level data to ThingsBoard
  tb.sendTelemetryInt("MQ5_level", mq5_level);     //Send MQ5 level data to ThingsBoard
  tb.sendTelemetryInt("Flame_level", flame_level);     //Send Flame level Sensor data to ThingsBoard
  tb.sendTelemetryString("Temperature_level_s", alarm_str[dht_level[0]]);  //Send temperature data to ThingsBoard
  tb.sendTelemetryString("Humidity_level_s", alarm_str[dht_level[1]]);     //Send humidity data to ThingsBoard
  tb.sendTelemetryString("MQ2_level_s", alarm_str[mq2_level]);     //Send MQ2 level data to ThingsBoard
  tb.sendTelemetryString("MQ5_level_s", alarm_str[mq5_level]);     //Send MQ5 level data to ThingsBoard
  tb.sendTelemetryString("Flame_level_s", alarm_str[flame_level]);     //Send Flame level Sensor data to ThingsBoard
  tb.sendTelemetryFloat("Water_Pump_Speed", water_pump_speed);     //Send Water Pump Speed data to ThingsBoard
  tb.sendTelemetryFloat("Fan_Speed", fan_speed);     //Send Fan Speed data to ThingsBoard
  tb.loop();
}



//=============================================================
//                      Input Functions
//=============================================================
//========================= Sensors ===========================
//DHT22, MQ2, MQ5 & Flame Sensors
void update_sensor(void * parameter){ 
  for(;;){    
    //DHT22
    dht_data[0] = dht.readTemperature();    //store temperature in index 0
    dht_data[1] = dht.readHumidity();       //store humidity in index 1
    Serial.print("DHT22 sensor [");
    Serial.print("Temperature: ");
    Serial.print(dht_data[0]);
    Serial.print(" °C | Humidity: ");
    Serial.print(dht_data[1]);
    Serial.println(" %RH]");
    // Set temperature warning level
    if (isnan(dht_data[0])) dht_level[0] = 0;
    else if (dht_data[0] <  dht_level_th0[1]) dht_level[0] = 1;
    else if ((dht_data[0] >=  dht_level_th0[1]) && (dht_data[0] <  dht_level_th0[2])) dht_level[0] = 2;
    else dht_level[0] = 3;
    // Set humidity warning level
    if (isnan(dht_data[1])) dht_level[1] = 0;
    else if (dht_data[1] >  dht_level_th1[1]) dht_level[1] = 1;
    else if ((dht_data[1] <=  dht_level_th1[1]) && (dht_data[1] >  dht_level_th1[2])) dht_level[1] = 2;
    else dht_level[1] = 3;
    
    //MQ2 Smoke Sensor
    mq2_data = analogRead(MQ2PIN);
    Serial.print("MQ2 sensor: ");
    Serial.print(mq2_data);
    // Set smoke level
    if(mq2_data < mq2_level_th[1]) mq2_level = 1;
    else if ((mq2_data >= mq2_level_th[1]) && (mq2_data < mq2_level_th[2])) mq2_level = 2;
    else mq2_level = 3;
    
    //MQ5 Gas Sensor
    mq5_data = analogRead(MQ5PIN);
    Serial.print("\tMQ5 sensor: ");
    Serial.print(mq5_data);
    // Set gas level
    if(mq5_data < mq5_level_th[1]) mq5_level = 1;
    else if ((mq5_data >= mq5_level_th[1]) && (mq5_data < mq5_level_th[2])) mq5_level = 2;
    else mq5_level = 3;
    
    //Flame Sensor
    flame_data = analogRead(FlamePIN);
    Serial.print("\tFlame sensor: ");
    Serial.println(flame_data);

    if(flame_data > flame_level_th[1]) flame_level = 1;
    else if ((flame_data <= flame_level_th[1]) && (flame_data > flame_level_th[2])) flame_level = 2;
    else flame_level = 3;
    
    Serial.print("Temperature: ");
    Serial.print(alarm_str[dht_level[0]]);
    Serial.print("\tHumidity: ");
    Serial.print(alarm_str[dht_level[1]]);
    Serial.print("\tSmoke: ");
    Serial.print(alarm_str[mq2_level]);
    Serial.print("\tGas: ");
    Serial.print(alarm_str[mq5_level]);
    Serial.print("\tFlame: ");
    Serial.println(alarm_str[flame_level]);
    
    vTaskDelay(2000);
  }
}

//=============================================================
//                      Output Functions
//=============================================================

//level 1: (dht_level[0] == 1 || dht_level[1] == 1 || mq2_level == 1 || mq5_level == 1 || flame_level == 1)
//level 2: (dht_level[0] == 0 || dht_level[1] == 0  || dht_level[0] == 2 || dht_level[1] == 2 || mq2_level == 2 || mq5_level == 2 || flame_level == 2)
//level 3: (dht_level[0] == 3 || dht_level[1] == 3 || mq2_level == 3 || mq5_level == 3 || flame_level == 3)

//========================== Display ==========================
// Display to OLED SSD1306
void print_oled_init() {
  display.clearDisplay();      // Clear the buffer
  display.setTextSize(2);
  display.setTextColor(WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(0, 0);
  display.print("MAN KMARS");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Hello World!");
  display.setCursor(0,40);
  display.print("INITIALIZING...");
  display.setCursor(0, 50);
  display.print("Connecting to WiFi...");
  
  display.display();
}
void print_oled_display(void * parameter) {
  int display_page = 0; // 0 = Temperature and humidity, 1 = Gas/Smoke level, 3 = Warning!
  for (;;) {
    display.clearDisplay();      // Clear the buffer
    display.setTextSize(2);
    display.setTextColor(WHITE); // Draw white text
    display.invertDisplay(false);
    display.setCursor(0, 0);     // Start at top-left corner
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    // Not all the characters will fit on the display. This is normal.
    // Library will draw what it can and the rest will be clipped.
    if(display_page == 0){
      // display temperature
      display.print("T: ");
      display.print(String(dht_data[0]));
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
      display.print(String(dht_data[1]));
      display.print(" %"); 
      display_page = 1;
    }
    else if(display_page == 1){
      // display temperature
      display.print("T: ");
      display.print(String(alarm_str[dht_level[0]]));
      // display humidity
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print("H: ");
      display.print(String(alarm_str[dht_level[1]]));
      display_page = 2;
    }
    else if(display_page == 2){
      // display mq2
      display.print("Smoke:");
      display.print(String(mq2_data));
      // display mq5
      display.setCursor(0, 25);
      display.print("Gas:");
      display.print(String(mq5_data));
      // display flame
      display.setCursor(0, 50);
      display.print("Flame:");
      display.print(String(flame_data));
      display_page = 3;
    }
    else if(display_page == 3){
      // display mq2
      display.print("Smoke:");
      display.print(String(alarm_str[mq2_level]));
      // display mq5
      display.setCursor(0, 25);
      display.print("Gas:");
      display.print(String(alarm_str[mq5_level]));
      // display flame
      display.setCursor(0, 50);
      display.print("Flame:");
      display.print(String(alarm_str[flame_level]));
      display_page = 0;
    }
    else{
      display.println("Test");
      display_page = 0;
    }
    display.display();

    // Blink Level for Warning
    if (dht_level[0] == 3 || dht_level[1] == 3 || mq2_level == 3 || mq5_level == 3 || flame_level == 3){
      display.invertDisplay(true);
      vTaskDelay(250); 
      display.invertDisplay(false);
      vTaskDelay(250); 
      display.invertDisplay(true);
      vTaskDelay(250); 
      display.invertDisplay(false);
      vTaskDelay(250); 
    }
    else if (dht_level[0] == 2 || dht_level[1] == 2 || mq2_level == 2 || mq5_level == 2 || flame_level == 2){
      display.invertDisplay(true);
      vTaskDelay(500); 
      display.invertDisplay(false);
      vTaskDelay(500); 
      display.invertDisplay(true);
    }
    else{
      vTaskDelay(1000);    
    }
  }
}


//============================ Alarm ===========================
void buzz_alarm(void * parameter){      // Alarm sound
  for (;;) {
    if (dht_level[0] == 3 || dht_level[1] == 3 || mq2_level == 3 || mq5_level == 3 || flame_level == 3){
       for(int i = 2000; i<=5000; i+=50){
         ledcWriteTone(BuzzCH, i);
         vTaskDelay(10);
       }
       for(int i = 0; i<=25; i++){
         ledcWriteTone(BuzzCH, 4000);
         vTaskDelay(25);
         ledcWriteTone(BuzzCH, 0);
         vTaskDelay(25);
       }
    }
    else if (dht_level[0] == 2 || dht_level[1] == 2 || mq2_level == 2 || mq5_level == 2 || flame_level == 2){
        for(int i = 0; i<=2; i++){
          ledcWriteTone(BuzzCH, 4000);
          vTaskDelay(50);
          ledcWriteTone(BuzzCH, 6000);
          vTaskDelay(50);
        }
     ledcWriteTone(BuzzCH, 0);
     vTaskDelay(4900);
    }
    else{
      vTaskDelay(1000);
    }
  }
}

void ex_fan(void * parameter){      // Fan
  for (;;) {
    if (mq2_level == 3 || mq5_level == 3 || dht_level[0] >= 2) {
      fan_speed = 1;
      analogWrite(FanPIN, 255);
      vTaskDelay(1000);
    }
    else if (mq2_level == 2 || mq5_level == 2){
      fan_speed = 0.5;
      analogWrite(FanPIN, 127);
      vTaskDelay(1000);
    }
    else{
      fan_speed = 0;
      analogWrite(FanPIN, 0);
      vTaskDelay(1000);
    }
    Serial.print("Fan State: ");
    Serial.println(fan_speed);
  }
}

void water_pump(void * parameter){      // Water Pump
  for (;;) {
    if (flame_level == 3 && (dht_level[0] >= 2 || isnan(dht_data[0]))) {
      water_pump_speed = 1;
      analogWrite(WaterPumpPIN, water_pump_speed*255);
      vTaskDelay(10000);
    }
    else{
      water_pump_speed = 0;
      analogWrite(WaterPumpPIN, water_pump_speed*255);
      vTaskDelay(1000);
    }
    Serial.print("Water Pump State: ");
    Serial.println(water_pump_speed);
  }
}



//=============================================================
//                        Connectivity
//=============================================================
//===================== Wi-Fi Functions =======================
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

//======================== ThingsBoard ==========================
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
