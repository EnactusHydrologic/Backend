#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"


//Firebase stuff
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
#define WIFI_SSID ""                                              // change
#define WIFI_PASSWORD ""                                         //change
#define DATABASE_URL ""                                          //change
#define API_KEY ""                                              // change         
#define USER_EMAIL ""                                            // change 
#define USER_PASSWORD ""                                         //change 
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
int timestamp;
FirebaseJson json;
const char* ntpServer = "pool.ntp.org";
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;    // every 180000ms = 180s = 3min
                                      // 1000ms = 1s


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


//FL408 sensor stuff
// struct FL408
const int waterMeterPin = A0;
volatile int  pulse_frequency;
unsigned int  literperhour;
unsigned long currentTime, loopTime;
byte sensorInterrupt = A0;

void FL408_init() {
  pinMode(waterMeterPin, INPUT);
  attachInterrupt(sensorInterrupt, getFlow, FALLING);
  currentTime = millis();
  loopTime = currentTime;
}

// int FL408_getData(){
//   //find accurate frequency and understand how timing works
//   if(currentTime >= (loopTime + 1000))
//    {
//       loopTime = currentTime;
//       literperhour = (pulse_frequency * 60 / 7.5);
//       pulse_frequency = 0;
//       Serial.print(literperhour, DEC);
//       Serial.println(" Liter/hour");
//     }
// }


void getFlow ()
{
   pulse_frequency++;
}


//WiFi Stuff 
// Initialize WiFi
void initWiFi() {
  int status = WL_IDLE_STATUS;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (status != WL_CONNECTED) {
    status = WiFi.status();
    Serial.println(get_wifi_status(status));
    // Serial.print("wifi problem");
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

// std::string getFormattedDateTime() {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     // Failed to obtain time
//     return "Failed to obtain time";
//   }
  
//   // Format the date and military hour
//   char formattedDateTime[20]; // Assuming a maximum of 20 characters for the formatted string
//   sprintf(formattedDateTime, "%04d/%02d/%02d %02d:%02d:%02d",
//           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
//           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

//   return formattedDateTime;
// }



void setup() {
  // Set baud rate on serial monitor to 115200
  Serial.begin(9600); 
  initWiFi();                 // initialize WiFi
  FL408_init();
  Firebase_init();             // initialize Firebase

}

unsigned long oldTime;
unsigned long initialFlowValue;
unsigned long currentFlowValue;

unsigned long sampleTime;
unsigned long calibrationFactor = 4.5; 

// unsigned int getData() {
//     pulse_frequency = 0;
//     literperhour = 0;
//     oldTime = millis();
//     initialFlowValue = digitalRead(sensorInterrupt);
//     sampleTime = oldTime + 500; // sets sampling to 0.5 s
//     while(oldTime < sampleTime) {
//       oldTime = millis();
//       currentFlowValue = digitalRead(sensorInterrupt);
//       if (initialFlowValue != currentFlowValue) {
//         pulse_frequency++;
//         initialFlowValue = currentFlowValue;
//       }
//     }
//     literperhour = (2* pulse_frequency) / calibrationFactor;
// }

void loop() {
  //change how often sensor data is set 
  currentTime = millis();
  if(currentTime >= (loopTime + 1000))
   {
      loopTime = currentTime;
      literperhour = (pulse_frequency * 60 / 7.5);
      pulse_frequency = 0;
      Serial.print(literperhour, DEC);
      Serial.println(" Liter/hour");
  }

  oldTime = millis();
  initialFlowValue = digitalRead(sensorInterrupt);
  sampleTime = oldTime + 500; // sets sampling to 0.5 s
  while(oldTime < sampleTime) {
    oldTime = millis();
    currentFlowValue = digitalRead(sensorInterrupt);
       if (initialFlowValue != currentFlowValue) {
          pulse_frequency++;
         initialFlowValue = currentFlowValue;
      }
    }
  literperhour = (2* pulse_frequency) / calibrationFactor;

  Serial.printf("loop: ", literperhour);
   //change how often firebase data is set 
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    timestamp = getEpochTime();
    Serial.print("time: ");
    Serial.println(timestamp);
    Serial.printf("firebase: ", literperhour);


    parentPath = databasePath + "/" + String(timestamp);

    json.set(litPerMinPath.c_str(), literperhour);
    json.set(timePath.c_str(), getFormattedTime());
    //  Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) this sets the name of each entry under readings
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "~.~ sent data successfully ~.~" : fbdo.errorReason().c_str());
  }
  delay(1000);
}
