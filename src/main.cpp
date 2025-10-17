#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "thx.h"

#define MAX_DISTANCE 100

HardwareSerial _serial1(2);
THX_SENSOR sensor(_serial1);

const char* ssid = "NITHUB-ICT-HUB";
const char* passwd = "6666.2524#";

// Firebase credentials
#define API_KEY "AIzaSyBF3o9jzozoZt_ht-sQDgT2uAstzehDYPg"
#define DATABASE_URL "https://workplace-security-system-default-rtdb.firebaseio.com/" 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WebServer server(80);

// Pin definitions
const int echoPin = 4;
const int trigPin = 5;
const int buzzer_pin = 14;
const int redPin = 13;
const int greenPin = 12;

// const int lockPin;

NewPing sonar = NewPing(trigPin, echoPin, MAX_DISTANCE);

volatile bool isRegistering = false;
// Task declarations
void registerTask();
void recognizeTask(void *pvParameters);


void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  WiFi.begin(ssid, passwd);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected on local network");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("doorlock")) {
    Serial.println("mDNS started, visit http://doorlock.local/");
  }

  
  // pinMode(buttonPin1, INPUT_PULLUP);
  //pinMode(buttonPin2, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Web server route for registration
  server.on("/register", registerTask);
  server.begin();

 // Face recognition task
  xTaskCreate(recognizeTask, "RecognizeTask", 4096, NULL, 1, NULL);

  Serial.println("System ready");

}

void loop() {
  // put your main code here, to run repeatedly:4
  server.handleClient();
  vTaskDelay(pdMS_TO_TICKS(1000));
}


bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    vTaskDelay(pdMS_TO_TICKS(50));
    if (digitalRead(pin) == LOW) return true;
  }
  return false;
}

// REGISTER TASK
void registerTask() {
  int id;
  Serial.println("Received request: register...");
  isRegistering = true;
  digitalWrite(redPin, HIGH);
  unsigned long currentTime = millis();

  while ((millis() - currentTime) < 5000) {
    id = sensor.sendCommand("register");
    if (id > 0) {
      Serial.printf("Registered User: %d\n", id);
      String path = "/attendance/User_";
      path += String(id);
      // --- Log registration to Firebase ---
      Firebase.RTDB.setString(&fbdo, (path + "/status").c_str(), "registered");
      Firebase.RTDB.setTimestamp(&fbdo, (path + "/timestamp").c_str());
      // --- Response to client ---
      String jsonResponse = "{\"status\":\"ok\", \"user_id\":";
      jsonResponse += String(id);
      jsonResponse += "}";
      server.send(200, "application/json", jsonResponse);

      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(greenPin, LOW);
      break;
    } else {
      Serial.println("Error occurred, trying again...");
    }
  }

  digitalWrite(redPin, LOW);
  if (id < 0) {
    Serial.println("Could not register user");
    server.send(404, "application/json",
                "{\"status\":\"failed\", \"error\":\"Could not register user\"}");
  }

  isRegistering = false;
}

// RECOGNIZE TASK
void recognizeTask(void *pvParameters){
  while(!isRegistering){
    int _distance = sonar.ping_cm();
    if(((_distance > 0) && _distance >= 25) && (_distance <= 75)){
        digitalWrite(redPin, HIGH);
        Serial.println("Object detected, analyzing...");
        int id = sensor.sendCommand("recognize");
        if(id >= 0){
          Serial.printf("recognized User: %d\n", id);
          digitalWrite(redPin, LOW);
          digitalWrite(greenPin, HIGH);

          // --- Activate buzzer briefly ---
          tone(buzzer_pin, 1000, 300);

          // --- Log recognition to Firebase ---
          String path = "/attendance/User_";
          path += String(id);

          String statusPath = path + "/status";
          String timePath = path + "/timestamp";
          Firebase.RTDB.setString(&fbdo, statusPath.c_str(), "recognized");
          Firebase.RTDB.setTimestamp(&fbdo, timePath.c_str());


          // digitalWrite(lockPin, HIGH);
          vTaskDelay(pdMS_TO_TICKS(2000));
          digitalWrite(greenPin, LOW);
          // digitalWrite(lockPin, LOW);
        }
        else{
          Serial.printf("Object not recognized");
          vTaskDelay(pdMS_TO_TICKS(10));
          digitalWrite(redPin, LOW);
        }
  }
  vTaskDelay(pdMS_TO_TICKS(500));
}
}