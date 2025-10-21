#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "thx.h"
#include "dashboard.h"

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
const int echoPin = GPIO_NUM_22;
const int trigPin = GPIO_NUM_23;
const int buzzer_pin = GPIO_NUM_14;
const int redPin = GPIO_NUM_13;
const int greenPin = GPIO_NUM_12;
const int lockPin = GPIO_NUM_1;

NewPing sonar = NewPing(trigPin, echoPin, MAX_DISTANCE);
volatile bool isRegistering = false;

// Function declarations
void registerTask();
void recognizeTask(void *pvParameters);
bool buttonPressed(int pin);
void deleteUser(int id);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected on local network: ");
  Serial.println(WiFi.localIP());

  // Start mDNS
  if (MDNS.begin("doorlock")) {
    Serial.println("mDNS started, visit http://doorlock.local/");
  }

  // Web routes
  server.on("/", []() {
    Serial.println("got a user");
    server.send_P(200, "text/html", index_html);
  });
  server.on("/register", []() { registerTask(); });
  server.on("/delete", []() {
    int id = server.arg("id").toInt();
    deleteUser(id);
  });
  server.begin();

  // Pin setup
  pinMode(buzzer_pin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(lockPin, OUTPUT);

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start recognition task
  xTaskCreate(recognizeTask, "RecognizeTask", 8192, NULL, 1, NULL);

  Serial.println("System ready");
}

void loop() {
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
      String path = "/attendance/User_" + String(id);

      // Log registration to Firebase
      Firebase.RTDB.setString(&fbdo, path + "/status", "registered");
      Firebase.RTDB.setTimestamp(&fbdo, path + "/timestamp");

      // Response to client
      String jsonResponse = "{\"status\":\"ok\", \"user_id\":" + String(id) + "}";
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
    server.send(404, "application/json", "{\"status\":\"failed\", \"error\":\"Could not register user\"}");
  }

  isRegistering = false;
}

// RECOGNIZE TASK
void recognizeTask(void *pvParameters) {
  while (!isRegistering) {
    unsigned long _distance = sonar.ping_cm();
    Serial.printf("%d\n", _distance);
    if ((_distance >= 15) && (_distance <= 75)) {
      digitalWrite(redPin, HIGH);
      Serial.println("Object detected, analyzing...");
      int id = sensor.sendCommand("recognize");
      if (id >= 0) {
        Serial.printf("Recognized User: %d\n", id);
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);

        // --- Activate buzzer briefly ---
        tone(buzzer_pin, 1000, 300);

        // --- Log recognition to Firebase ---
        String path = "/attendance/User_" + String(id);
        Firebase.RTDB.setString(&fbdo, path + "/status", "recognized");
        Firebase.RTDB.setTimestamp(&fbdo, path + "/timestamp");

        vTaskDelay(pdMS_TO_TICKS(2000));
        digitalWrite(greenPin, LOW);
      } else {
        Serial.println("Object not recognized");
        vTaskDelay(pdMS_TO_TICKS(10));
        digitalWrite(redPin, LOW);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

void deleteUser(int id) {
  Serial.println("Deleting user...");
  int res = sensor.sendCommand("delete", id);
  if (res >= 0) {
    server.send(200, "application/json", "{\"status\":\"ok\", \"user_id\":" + String(res) + "}");
  } else {
    server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"Could not delete user\"}");
  }
}
