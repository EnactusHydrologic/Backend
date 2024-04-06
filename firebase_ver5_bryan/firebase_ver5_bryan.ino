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
#include <esp_task_wdt.h>


#define WIFI_SSID "IllinoisNet_Guest"                                              // change
#define WIFI_PASSWORD ""                                         //change
#define DATABASE_URL "https://hydrobryan-608df-default-rtdb.firebaseio.com/"                                          //change
#define API_KEY "AIzaSyBAHEZcJ8F1WiusYmaw_8rZD2exXFwVW2I"                                              // change
#define USER_EMAIL "mvp1@gmail.com"                                            // change
#define USER_PASSWORD "00000007"                                         //change
#define WDT_TIMEOUT 30
#define STORAGE_BUCKET_ID "hydrobryan-608df.appspot.com"
#define CURR_VERSION_NUM 1

int avaliable_version_num = 0;

FirebaseData fbdo; // object for authentification
FirebaseAuth auth;
FirebaseConfig config;  // object for data configuration
String uid; // user UID
String databasePath;  // database main path
                    // update later with user UID
String timePath = "/timestamp";
String litPerMinPath = "/litPerMin";  // child node
String batteryVoltPath = "/batteryVolt";  // child node
String parentPath; // parent node that is updated in every loop with timestamp
/*
*                               |-child-|
*|---parentPath----------------|
*|-databasePath-----|
*UserData/<user_uid*>/timestamp/litPerMin
*/
//cannot be set above 512 
#define BUFFER_SIZE 400

int timestamp[BUFFER_SIZE];
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 3000;    // every 180000ms = 180s = 3min
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
  
  fbdo.setResponseSize(8192);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 8192 /* Tx buffer size in bytes from 512 - 16384 */);
  config.token_status_callback = tokenStatusCallback; // see addonsTokenHelper.h
  config.max_token_generation_retry = 5;
  // initialize firebase with config and authentification
  Firebase.begin(&config, &auth);
  Firebase.RTDB.setMaxRetry(&fbdo, 5);
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
  //Dorm/Floor/
  //databasePath = "/UsersData/" + uid + "/readings";
  databasePath = "Dorms/ISR/1stFloor/"+ uid ;
}
//initialize the water flow sensor
void FL408_init() {
  pinMode(waterMeterPin, INPUT_PULLDOWN);
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
  return (rtc.getLocalEpoch() / 86400) + 25569;
}


// The Firebase Storage download callback function
void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
    if (info.status == firebase_fcs_download_status_init)
    {
        Serial.printf("Downloading firmware %s (%d)\n", info.remoteFileName.c_str(), info.fileSize);
    }
    else if (info.status == firebase_fcs_download_status_download)
    {
        Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_download_status_complete)
    {
        Serial.println("Update firmware completed.");
        Serial.println();
        Serial.println("Restarting...\n\n");
        delay(2000);
        ESP.restart();
    }
    else if (info.status == firebase_fcs_download_status_error)
    {
        Serial.printf("Download firmware failed, %s\n", info.errorMsg.c_str());
    }
}

void setup() {
  // Set baud rate on serial monitor to 115200
  Serial.begin(9600);
  initWiFi();                 // initialize WiFi
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  FL408_init();
  analogReadResolution(12);

  Firebase_init();             // initialize Firebase

  //configTime(0,0, ntpServer); // configure time
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

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
  Serial.printf("\nCurrent Firmware version: %d\n",CURR_VERSION_NUM);
  Serial.println("\nChecking for Firmware update\n");

  // Check for firmware update
  if (Firebase.ready()){
    Serial.printf("List files... %s\n", Firebase.Storage.listFiles(&fbdo, STORAGE_BUCKET_ID) ? "ok" : fbdo.errorReason().c_str());

        if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK)
        {
            FileList *files = fbdo.fileList();
            for (size_t i = 0; i < files->items.size(); i++){
                Serial.printf("name: %s, bucket: %s\n", files->items[i].name.c_str(), files->items[i].bucket.c_str());
                int version_num;
                sscanf(files->items[i].name.c_str(),"%*[^0-9]%d",&version_num);
                if (version_num > avaliable_version_num){
                  avaliable_version_num = version_num;
                }
                Serial.printf("\nAvaliable version number: %d\n",version_num);
            }
        }
        // To clear file list
        fbdo.fileList()->items.clear();

  }
  if (avaliable_version_num>CURR_VERSION_NUM){
    Serial.println("newer firmware avaliable");
    if (!Firebase.Storage.downloadOTA(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, "firmware"+std::to_string(avaliable_version_num)+".bin" /* path of firmware file stored in the bucket */, fcsDownloadCallback /* callback function */))
      Serial.println(fbdo.errorReason());
  }else {
    Serial.println("Current Firmware is the newest or update firmware not found");
  }
  /////////////////////////////done checking for firmware update////////////////////////////////////////////
  

  
  
      
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
    Serial.printf("Buffer index: %d\n", bufferIndex);
    

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
        // int retries = 0;
        // while (retries < 5) {
        //   bool sentDataSuccess = Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr);
        //   delay(500);
        //   if (sentDataSuccess||fbdo.errorReason().c_str()==" ") {
        //     Serial.println("sent data sucessfully");
        //     break;  // If successful, break the loop
        //   } else {
        //     Serial.println("sending data to Firebase failed");
        //     Serial.println(fbdo.errorReason().c_str());
        //     retries++;
        //     delay(1000);  // Delay before retrying
        //   }
        // }
        Serial.printf("Set json... %s\n", Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr) ? "~.~ sent data successfully ~.~" : fbdo.errorReason().c_str());
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
        
          if (i == bufferIndex-idle_time-1){
            json.set(batteryVoltPath.c_str(), 2*analogRead(A0)*(3.3/4095));
          }
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
            // int retries = 0;
            //  while (retries < 5) {
            //   bool sentDataSuccess = Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr);
            //   delay(500);
            //   if (sentDataSuccess) {
            //     Serial.println("sent data sucessfully");
            //     break;  // If successful, break the loop
            //   } else {
            //     Serial.println("sending data to Firebase failed");
            //     Serial.println(fbdo.errorReason().c_str());
            //     retries++;
            //     delay(1000);  // Delay before retrying
            //   }
            // }
            Serial.printf("Set json... %s\n", Firebase.RTDB.setArray(&fbdo, parentPath.c_str(), &jsonArr) ? "~.~ sent data successfully ~.~" : fbdo.errorReason().c_str());
          }
        }
        
        // Reset the buffer index
        bufferIndex = 0;
        //////////////////////done sending/////////////////
        //check the status of the sensor and set the corresponding wake up time
        Serial.printf("setting sensor status to %d", !digitalRead(waterMeterPin));
        esp_sleep_enable_ext0_wakeup(waterMeter_gpio,!digitalRead(waterMeterPin));
        Serial.printf("Battery Voltage: %f", analogRead(A0)*(3.3/4095));
        sleepCount = 0;
        Serial.println(" sleeping...");
        delay(1500);
        esp_deep_sleep_start();
      }
    } else {
      sleepCount = 0;
    }
    esp_task_wdt_reset();
  }
  delay(100);
  

    
}