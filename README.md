# Hydrologic

## I.1 Arduino IDE setup for sending data to Firebase
Tutorial I followed: https://randomnerdtutorials.com/esp32-data-logging-firebase-realtime-database/
1. Install ArduinoIDE \n
2. Install the Firebase ESP Client Library
   through Tools->Manage Libraries->"Firebase Arduino Client Library for ESP8266 and ESP32" \n
3. Install ESP32 drivers through
   Tools->Board->Board Manager->"esp32 by Espressif Systems" \n

## I.2 ESP32 -> WiFi connection (Directly from tutorial)
1. Change WIFI_SSID to the WiFi
2. Change WIFI_PASSWORD to the WiFi passoword
3. Make sure the WiFi selected is 2.4GHz, otherwise ESP32 doesn't recognize it

## I.3 ESP32 -> Firebase (Directly from tutorial)
1. Change DATABASE_URL to the URL from your database from the Realtime Database dashboard
2. Change API_KEY to your API_KEY from previous step.
3. Change USER_EMAIL and USER_PASSWORD to your authentification method

## II.1 VSCode setup for receiving data from Firebase
Tutorial I partially followed : https://randomnerdtutorials.com/esp32-esp8266-firebase-gauges-charts/
1. Install VSCode \n
2. Install Python
3. Install Firebase
4. Change databaseURL from Firebase->Python SDK->databaseURL

## II.2 VSCode setup for Streamlit
1. Install Streamlit (make sure you have Python too)
2. To run it, type python -m streamlit run .\main_page.py
### Example: 
PS C:\Users\Administrator\Desktop\Hydrologic_WIN23\Fa2023Hydro> python -m streamlit run .\main_page.py

  You can now view your Streamlit app in your browser.  

  Local URL: http://localhost:8501
  Network URL: http://192.168.0.27:8501
