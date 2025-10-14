#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "thx.h"
#include "dashboard.h"


#define MAX_DISTANCE 100

HardwareSerial _serial1(2);
THX_SENSOR sensor(_serial1);

const char* ssid = "NITDA-ICT-HUB";
const char* passwd = "6666.2524#";
WebServer server(80);

const int echoPin = GPIO_NUM_4;
const int trigPin = GPIO_NUM_5;

//const int buttonPin1 = 15;
//const int buttonPin2 = 13;

const int redPin = GPIO_NUM_13;
const int greenPin = GPIO_NUM_12;

//const int lockPin;



NewPing sonar = NewPing(trigPin, echoPin, MAX_DISTANCE);


void registerTask();
void recognizeTask(void *pvParameters);
bool buttonPressed(int pin);

volatile bool isRegistering = false;

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
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
  server.on("/register", [](){registerTask();});

  server.begin();

  //pinMode(buttonPin1, INPUT_PULLUP);
  //pinMode(buttonPin2, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

//xTaskCreate(registerTask, "RegisterTask", 4096, NULL, 1, NULL);
 // xTaskCreate(recognizeTask, "RecognizeTask", 4096, NULL, 1, NULL);

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
  int id;
  Serial.println("Recieved request: register...");
  isRegistering = true;
  digitalWrite(redPin, HIGH);
  unsigned long currentTime = millis();
  while((millis() - currentTime) < 5000){
      id = sensor.sendCommand("register");
      if(id >= 0){
        Serial.printf("registered User: %d\n", id);
        server.send(200, "application/json", "{\"status\":\"ok\", \"user_id\":" + String(id) + "}");
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(1500));
        digitalWrite(greenPin, LOW);
        break;
      }
      else{
        Serial.println("Error Occured, trying again...");
      }
  }
  digitalWrite(redPin, LOW);
  if(id < 0){
  Serial.println("Could not register User");
  server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"" + err_code[id] + "\"}");
  }
  isRegistering = false;
}

void recognizeTask(void *pvParameters){
  if(false){
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
          //digitalWrite(lockPin, HIGH);
          vTaskDelay(pdMS_TO_TICKS(2000));
          digitalWrite(greenPin, LOW);
          //digitalWrite(lockPin, LOW);
        }
        else{
          Serial.printf("Object not recognized");
          vTaskDelay(pdMS_TO_TICKS(10));
          digitalWrite(redPin, LOW);
        }
  }
  vTaskDelay(pdMS_TO_TICKS(500));
}}
}