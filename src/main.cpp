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

const char* ssid = " ";
const char* passwd = " ";
WebServer server(80);

const int echoPin = GPIO_NUM_22;
const int trigPin = GPIO_NUM_23;

//const int buttonPin1 = 15;
//const int buttonPin2 = 13;

const int redPin = GPIO_NUM_13;
const int greenPin = GPIO_NUM_12;

const int lockPin = GPIO_NUM_1;



NewPing sonar = NewPing(trigPin, echoPin, MAX_DISTANCE);

volatile bool isRegistering = false;


void registerTask();
void recognizeTask(void *pvParameters);
bool buttonPressed(int pin);
void deleteUser(int id);

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
  xTaskCreate(recognizeTask, "RecognizeTask", 8192, NULL, 1, NULL);

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
  Serial.println("Could not register User");
  server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"" + err_code[id] + "\"}");
  }
  isRegistering = false;
}

void recognizeTask(void *pvParameters){
  while(!isRegistering){
    unsigned long _distance = sonar.ping_cm();
    Serial.printf("%d\n", _distance);
    if((_distance >= 15) && (_distance <= 75)){
        digitalWrite(redPin, HIGH);
        Serial.println("Object detected, analyzing...");
        int id = sensor.sendCommand("recognize");
        if(id >= 0){
          Serial.printf("recognized User: %d\n", id);
          digitalWrite(redPin, LOW);
          digitalWrite(greenPin, HIGH);
          digitalWrite(lockPin, HIGH);
          vTaskDelay(pdMS_TO_TICKS(2000));
          digitalWrite(greenPin, LOW);
          digitalWrite(lockPin, LOW);
        }
        else{
          Serial.printf("Object not recognized");
          vTaskDelay(pdMS_TO_TICKS(10));
          digitalWrite(redPin, LOW);
        }
  }
  vTaskDelay(pdMS_TO_TICKS(10000));
}}


void deleteUser(int id){
  Serial.println("Deleting user...");
  int res = sensor.sendCommand("delete", id);
  if(res >= 0){
    server.send(200, "application/json", "{\"status\":\"ok\", \"user_id\":" + String(res) + "}");
  }
  else{
    server.send(200, "application/json", "{\"status\":\"failed\", \"error\":\"" + err_code[res] + "\"}");
  }
}



/*const int lockPin = GPIO_NUM_27;

void setup(){
  pinMode(lockPin, OUTPUT);
  Serial.begin(115200);
}

void loop(){
  digitalWrite(lockPin, HIGH);
  Serial.println("High");
  delay(2000);
  digitalWrite(lockPin, LOW);
  Serial.println("Low");
  delay(2000);
}*/