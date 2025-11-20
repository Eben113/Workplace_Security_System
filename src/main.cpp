#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "thx.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "dashboard.h"
#include <freertos/semphr.h>
#include <ArduinoJson.h>


#define MAX_DISTANCE 100

HardwareSerial _serial1(2);
THX_SENSOR sensor(_serial1);

const char* ssid = "WOSEMUNILAG IPEXPRESS FIBER"; 
//"Ayo";
//"NITDA-ICT-HUB";
const char* passwd = "Wos1987@unilag"; 
//"ebenezer";
//"6666.2524#";

#define API_KEY "AIzaSyBF3o9jzozoZt_ht-sQDgT2uAstzehDYPg"
#define DATABASE_URL "https://workplace-security-system-default-rtdb.firebaseio.com/" 
#define DATABASE_SECRET "2XAq3MMZr8moaEZrPapn7gwweFX0IIo3NDMWfHft"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

SemaphoreHandle_t serialMutex;
SemaphoreHandle_t firebaseMutex;
SemaphoreHandle_t sensorMutex;


WebServer server(80);

const int echoPin = GPIO_NUM_22;
const int trigPin = GPIO_NUM_23;

//const int buttonPin1 = 15;
//const int buttonPin2 = 13;

const int buzzer_pin = GPIO_NUM_14;
const int redPin = GPIO_NUM_13;
const int greenPin = GPIO_NUM_12;

const int lockPin = GPIO_NUM_25;



NewPing sonar = NewPing(trigPin, echoPin, MAX_DISTANCE);

volatile bool isRegistering = false;


void registerTask();
void recognizeTask(void *pvParameters);
bool buttonPressed(int pin);
void deleteUser(int id);

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:

  serialMutex = xSemaphoreCreateMutex();
  firebaseMutex = xSemaphoreCreateMutex();
  sensorMutex = xSemaphoreCreateMutex();


  WiFi.begin(ssid, passwd);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected on local network\n");

  if (MDNS.begin("doorlock")) {
    Serial.println("mDNS started, visit http://doorlock.local/");
  }

  server.on("/", []() {
    Serial.println("got a user");
    server.send_P(200, "text/html", index_html);
  });

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  // Optional but good practice: show debug info
  config.token_status_callback = tokenStatusCallback; // from TokenHelper.h

  // No email/password because we’re signing in anonymously
  //auth.user.email = "";
  //auth.user.password = "";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  server.on("/register", [](){registerTask();});

  server.on("/delete", ([](){
    int id = server.arg("id").toInt(); 
    deleteUser(id);
  }));

  server.begin();

  //pinMode(buttonPin1, INPUT_PULLUP);
  //pinMode(buttonPin2, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(lockPin, OUTPUT);

//xTaskCreate(registerTask, "RegisterTask", 4096, NULL, 1, NULL);
  xTaskCreate(recognizeTask, "RecognizeTask", 10000, NULL, 1, NULL);

  Serial.println("System ready");

}

void loop() {
  // put your main code here, to run repeatedly:4
  server.handleClient();
  //vTaskDelay(pdMS_TO_TICKS(1000));
}


bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    vTaskDelay(pdMS_TO_TICKS(50));
    if (digitalRead(pin) == LOW) return true;
  }
  return false;
}

void registerTask(){
  isRegistering = true; 
  Serial.printf("Registering...");

  if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.println("Recieved request: register...");
          xSemaphoreGive(serialMutex);
        }

  digitalWrite(redPin, HIGH);
  unsigned long currentTime = millis();
  int id = -1;
  while((millis() - currentTime) < 5000){
      String name;

      if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE) {
          id = sensor.sendCommand("register");
          xSemaphoreGive(sensorMutex);
        }
      
      if (server.hasArg("name")){
        name = server.arg("name");
      }
      
      if(id >= 0){

        if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.printf("registered User: %d\n", id);
          xSemaphoreGive(serialMutex);
        }

        String dir = "/status", dir1 = "/timestamp";
        //path += name;
        //path += dir;
        String status = "registered";

        if(xSemaphoreTake(firebaseMutex, portMAX_DELAY) == pdTRUE){
          // --- Log registration to Firebase ---
          String path = "/attendance/";
          path += id;

          FirebaseJson json;
          json.set("name", name.c_str());
          //json.set("last_seen", (const char*)"{\"/.sv\":\"timestamp\"}");

          Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);

          String timestamp_path = path  + "/last_seen";
          Firebase.RTDB.setTimestamp(&fbdo, timestamp_path.c_str());

          //Firebase.RTDB.setString(&fbdo, path.c_str(), status.c_str());
          //Firebase.RTDB.setTimestamp(&fbdo, (path + dir1).c_str());
          xSemaphoreGive(firebaseMutex);
          }
        
        StaticJsonDocument<64> doc;
        doc["status"] = "ok";
        doc["user_id"] = id;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);

        Serial.printf(jsonResponse.c_str());


        server.send(200, "application/json", jsonResponse);
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
        delay(1500);
        digitalWrite(greenPin, LOW);
        break;
      }
      else{
        Serial.println("Error Occured, trying again...");
      }
  }
  digitalWrite(redPin, LOW);
  if(id < 0){

  if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.println("Could not register User");
          xSemaphoreGive(serialMutex);
        }
  
  server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"" + err_code[id] + "\"}");
  }
  isRegistering = false;
}

void recognizeTask(void *pvParameters){
  while(true){
    if(!isRegistering){
    unsigned long _distance = sonar.ping_cm();

    if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
    Serial.printf("%d\n", _distance);
    xSemaphoreGive(serialMutex);
  }


    if((_distance >= 15) && (_distance <= 75)){
        digitalWrite(redPin, HIGH);

        if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
        Serial.println("Object detected, analyzing...");
        xSemaphoreGive(serialMutex);
        }

        int id = -1;

        if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE) {
          id = sensor.sendCommand("recognize");
          xSemaphoreGive(sensorMutex);
        }


        if(id >= 0){
          if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.printf("recognized User: %d\n", id);
          xSemaphoreGive(serialMutex);
        }

          digitalWrite(redPin, LOW);
          digitalWrite(greenPin, HIGH);
          digitalWrite(lockPin, HIGH);
          // --- Activate buzzer briefly ---
          tone(buzzer_pin, 1000, 300);

          // --- Log recognition to Firebase ---
          String path = "/attendance/";
          path += id;


          if(xSemaphoreTake(firebaseMutex, portMAX_DELAY) == pdTRUE){
          String timestamp_path = path  + "/last_seen";
          Firebase.RTDB.setTimestamp(&fbdo, timestamp_path.c_str());
          xSemaphoreGive(firebaseMutex);
          }


          // digitalWrite(lockPin, HIGH);
          vTaskDelay(pdMS_TO_TICKS(2000));
          digitalWrite(greenPin, LOW);
          digitalWrite(lockPin, LOW);
        }
        else{
          if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.printf("Object not recognized");
          xSemaphoreGive(serialMutex);
        }

          vTaskDelay(pdMS_TO_TICKS(10));
          digitalWrite(redPin, LOW);
        }
  }
}
  vTaskDelay(pdMS_TO_TICKS(5000));

}
}


void deleteUser(int id){
  if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.println("Deleting user...");
          xSemaphoreGive(serialMutex);
        }
  
  int res = -1;
  if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE) {
          res = sensor.sendCommand("delete", id);
          xSemaphoreGive(sensorMutex);
        }

  if(res >= 0){
    String path = "/attendance/";
    path += String(id);
    if(xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE){
          Serial.printf(path.c_str());
          xSemaphoreGive(serialMutex);
        }
    if (xSemaphoreTake(firebaseMutex, portMAX_DELAY) == pdTRUE) {
          if (Firebase.RTDB.deleteNode(&fbdo, path.c_str())) {
            Serial.printf("Successfully deleted node at: %s\n", path.c_str());
           } else {
               Serial.printf("Failed to delete node. Reason: %s\n", fbdo.errorReason().c_str());
          }
          xSemaphoreGive(firebaseMutex);
    }

    server.send(200, "application/json", "{\"status\":\"ok\", \"user_id\":" + String(res) + "}");
  }
  else{
    server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"" + err_code[res] + "\"}");
  }
}
