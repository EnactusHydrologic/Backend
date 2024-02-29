#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"
#include <ESP32Time.h>
//Firebase stuff
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
#define WIFI_SSID "Happy Illini"                                              // change
#define WIFI_PASSWORD "00000007"                                         //change
#define DATABASE_URL "https://hydrobryan-608df-default-rtdb.firebaseio.com/"                                          //change
#define API_KEY "AIzaSyBAHEZcJ8F1WiusYmaw_8rZD2exXFwVW2I"                                              // change
#define USER_EMAIL "bryanchang1234@gmail.com"                                            // change
#define USER_PASSWORD "@B29908719b"                                         //change


FirebaseData fbdo; // object for authentification
FirebaseAuth auth;
FirebaseConfig config;  // object for data configuration
String uid; // user UID
String databasePath;  // database main path
                    // update later with user UID
String timePath = "/timestamp";
String litPerMinPath = "/litPerMin";  // child node
String parentPath; // parent node that is updated in every loop with timestamp
/*
*                               |-child-|
*|---parentPath----------------|
*|-databasePath-----|
*UserData/<user_uid*>/timestamp/litPerMin
*/
#define BUFFER_SIZE 25
int timestamp[BUFFER_SIZE];
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;    // every 180000ms = 180s = 3min
//time after there is no water flow to put the device into sleep
#define idle_time 6
int sleepCount = 0;

const int waterMeterPin = 13;
volatile int  pulse_frequency;
unsigned int  literperhour;
unsigned long currentTime, loopTime;
byte sensorInterrupt = 13;
gpio_num_t waterMeter_gpio = GPIO_NUM_13;

//RTC time 
ESP32Time rtc(0); 

// initialize firebase server, set up user, set db path
void Firebase_init() {
  configTime(0,0, ntpServer); // configure time
  config.api_key = API_KEY;   // configure API
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback; // see addonsTokenHelper.h
  config.max_token_generation_retry = 5;
  // initialize firebase with config and authentification
  Firebase.begin(&config, &auth);
  Serial.println("Getting user UID");
  while ((auth.token.uid) == "") {
    // Serial.print("here");
    Serial.print('.');
    delay(1000);
  }
  // print UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID:");
  Serial.println(uid);
  // update database path
  databasePath = "/UsersData/" + uid + "/readings";
}
//initialize the water flow sensor
void FL408_init() {
  pinMode(waterMeterPin, INPUT);
  attachInterrupt(sensorInterrupt, getFlow, FALLING);
  // currentTime = millis();
  // loopTime = currentTime;
}
//use to be call when water meter sensorInterrupt
void getFlow ()
{
   pulse_frequency++;
}
// Initialize WiFi
void initWiFi() {
  // Check if the device is already connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi.");
    Serial.println(WiFi.localIP());
    return;
  }
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (status != WL_CONNECTED) {
    status = WiFi.status();
    Serial.println(get_wifi_status(status));
    //Serial.print("wifi problem");
    delay(500);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
  
}
String get_wifi_status(int status){
    switch(status){
        case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
        case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
        case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
        case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
        case WL_CONNECTED:
        return "WL_CONNECTED";
        case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    }
}
// Synchronization stuff
// Function that gets current epoch time
unsigned long getEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
unsigned long getFormattedTime() {
  return (getEpochTime() / 86400) + 25569;
}

void setup() {
  // Set baud rate on serial monitor to 115200
  Serial.begin(9600);
  initWiFi();                 // initialize WiFi
  FL408_init();

  Firebase_init();             // initialize Firebase

  //configTime(0,0, ntpServer); // configure time
  ///////////////setting RTC///////////////////////////
  Serial.println("Setting RTC timer to NTP: ");
  int retries = 0;
  unsigned long currEpochTime = getEpochTime();
  while (retries < 5) {
    if (currEpochTime==0) {
      currEpochTime = getEpochTime();
      retries++;
    } else {
      rtc.setTime(currEpochTime); 
      Serial.println("set RTC sucessfully");
      break;  // If successful, break the loop
    }
  }
  //////////////done setting RTC//////////////////

  esp_sleep_enable_ext0_wakeup(waterMeter_gpio,1);

  //turn off wifi
  Serial.println("Turning WiFi off...");
  WiFi.mode(WIFI_OFF);

}

unsigned long calibrationFactor = 4.5;

int buffer[BUFFER_SIZE];
unsigned long timestamps[BUFFER_SIZE];
int bufferIndex = 0;

void loop() {
  literperhour = (2* pulse_frequency) / calibrationFactor;

  if ((millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    timestamp[bufferIndex] = rtc.getLocalEpoch();
    timestamps[bufferIndex] = rtc.getLocalEpoch();

    Serial.print("time: ");
    Serial.println(timestamps[bufferIndex]);
    Serial.printf("Litter per hour: %d\n", literperhour);

    // Store the data in the buffer
    buffer[bufferIndex] = literperhour;
    bufferIndex++;

    pulse_frequency = 0;

    // If the buffer is full, send the data to Firebase
    if (bufferIndex >= BUFFER_SIZE) {
      FirebaseJsonArray jsonArr;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        parentPath = databasePath + "/" + String(timestamp[i]);
        json.set(litPerMinPath.c_str(), buffer[i]);
        json.set(timePath.c_str(), timestamps[i]);
        jsonArr.add(json);
        
      }
      //connect to WiFi
      initWiFi();
      delay(1000);
      if(Firebase.ready()){
        int retries = 0;
        while (retries < 5) {
          if (!Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr)) {
            Serial.println(fbdo.errorReason().c_str());
            retries++;
            delay(1000);  // Delay before retrying
          } else {
            Serial.println("sent data sucessfully");
            break;  // If successful, break the loop
          }
        }
      }
      // Reset the buffer index
      bufferIndex = 0;
      Serial.println("Turning WiFi off...");
      WiFi.mode(WIFI_OFF);
    }

    // If there's no water flow, sleep after idle_time
    if (literperhour == 0) {
      sleepCount++;
      if (sleepCount == idle_time) {
        FirebaseJsonArray jsonArr;
        //send the buffer of current session to firebase
        for (int i = 0; i < bufferIndex-idle_time; i++) {
        //json.set(String(timestamps[i]).c_str(), buffer[i]);
        parentPath = databasePath + "/" + String(timestamp[i]);
        json.set(litPerMinPath.c_str(), buffer[i]);
        json.set(timePath.c_str(), timestamps[i]);
        jsonArr.add(json);
        
        }
        if (bufferIndex-idle_time>0){
          // initWiFi();
          // // Firebase_init();
          // delay(1000);
          // if(Firebase.ready()){
          //   Serial.printf("Set json... %s\n", Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr) ? "~.~ sent data successfully ~.~" : fbdo.errorReason().c_str());
          // }
          initWiFi();
          delay(1000);
          if(Firebase.ready()){
            int retries = 0;
            while (retries < 5) {
              if (!Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr)) {
                Serial.println(fbdo.errorReason().c_str());
                retries++;
                delay(1000);  // Delay before retrying
              } else {
                Serial.println("sent data sucessfully");
                break;  // If successful, break the loop
              }
            }
          }
        }
        
        // Reset the buffer index
        bufferIndex = 0;
        //////////////////////done sending/////////////////
        sleepCount = 0;
        Serial.println(" sleeping...");
        delay(1500);
        esp_deep_sleep_start();
      }
    } else {
      sleepCount = 0;
    }
  }
  delay(500);
  

    
}