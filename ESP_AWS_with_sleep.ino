/* Added */
/* AWS Stuff */
#include "Secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
//time after there is no water flow to put the device into sleep
#define idle_time 10

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

const int watermeterPin = 13;
volatile int  pulse_frequency;
unsigned int  literperhour;
unsigned long currentTime, loopTime;

byte sensorInterrupt = 13;

int sleepCount = 0;
gpio_num_t waterMeterGPIO = GPIO_NUM_13;

void setup()
{
   Serial.begin(115200);
   connectAWS();
   
   pinMode(watermeterPin, INPUT);
   attachInterrupt(sensorInterrupt, getFlow, FALLING);
   currentTime = millis();
   loopTime = currentTime;
   // init that if the water meter sensor send signal, wake up the esp32
   esp_sleep_enable_ext0_wakeup(waterMeterGPIO,1);
} 

void loop ()
{
   currentTime = millis();
   if(currentTime >= (loopTime + 1000))
   {
      loopTime = currentTime;
      literperhour = (pulse_frequency * 60 / 7.5);
      pulse_frequency = 0;
      Serial.print(literperhour, DEC);
      Serial.println(" Liter/hour");
      
      //if there's no water flow sleep after idle_time
      if(literperhour==0) {
        sleepCount++;
        if (sleepCount == idle_time) {
          sleepCount = 0;
          Serial.println(" sleeping...");
          esp_deep_sleep_start();
         }
      } else {
        sleepCount = 0;
      }
      
   }
    
   publishMessage();
   client.loop();
   delay(1000);
}

void getFlow ()
{
   pulse_frequency++;
}
void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["Liter/hour"] = literperhour;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}
