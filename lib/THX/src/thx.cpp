#include "thx.h"


std::map<int, String> err_code = {
    {-1,"No Face Detected"},
    {-3,"Pose angle is too large"},
    {-6 ,"2D live not passed"},
    {-8,"Match Failed"},
};

void clearSerialBuffer(HardwareSerial& port) {
    while (port.available() > 0) {
        port.read();
    }
}


THX_SENSOR::THX_SENSOR(HardwareSerial& serialPort) :_serial(serialPort){
    _serial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("THX Sensor initialized");
}


int THX_SENSOR::readOut(){
  while(1){
    if(_serial.available() > 7){
      Serial.println("Got Something, Checking...");
      uint8_t sync[2];
      sync[0] = _serial.read();
      sync[1] = _serial.read();
      uint16_t syncWord = (sync[0] << 8) | sync[1];

      if(syncWord == 0xEFAA){
        uint8_t MsgId = _serial.read();
        
        uint8_t sizeBytes[4];
        for (int i = 0; i < 4; i++) sizeBytes[i] = _serial.read();
        uint32_t size = (sizeBytes[0] << 24) | (sizeBytes[1] << 16) | (sizeBytes[2] << 8) | sizeBytes[3];
      }else{
        Serial.printf("syncword not recognized during readout: %hhu\n", syncWord);
        return -1;
      }
      uint8_t id = _serial.read();
      if (id == 0x12){
            uint8_t res = _serial.read();
            if(res == 0x00){
                uint8_t idBytes[2];
                idBytes[0] = _serial.read();
                idBytes[1] = _serial.read();
                uint16_t userId = (idBytes[0] << 8) | idBytes[1];
                Serial.printf("Recognized user ID: %d\n", userId);
                return userId;
            }
            else if (res == 0x01){
                Serial.printf("No face detected\n");
                return -1;}
          }
      else if (id == 0x13){
          uint8_t res = _serial.read();
          if(res == 0x00){
              uint8_t idBytes[2];
              idBytes[0] = _serial.read();
              idBytes[1] = _serial.read();
              uint16_t userId = (idBytes[0] << 8) | idBytes[1];
              Serial.printf("User registered successfully. ID: %d\n", userId);
              return userId;
          }
          else{
              Serial.printf(err_code[-1*(int)res].c_str());
              return -1* (int)res;}
          break;}
      else{
            if(_serial.read() == 0x00){
                Serial.printf("Command Successful\n");
            }
            else{
                Serial.printf("Error");}
            break;
      }
    }
    else{
        Serial.printf("No data yet, waiting....\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}



int THX_SENSOR::sendCommand(std::string command){
  uint8_t packet[8];
  int idx = 0;

  packet[idx++] = 0xEF;
  packet[idx++] = 0xAA;

  if(command == "register"){
    packet[idx++] = 0x13;
    packet[idx++] = 0x00; // Size (no payload)
    packet[idx++] = 0x00;
    packet[idx++] = 0x00;
    packet[idx++] = 0x00;
    packet[idx++] = 0x13;
  }
  else if (command == "recognize")
  {
    packet[idx++] = 0x12;
    packet[idx++] = 0x00;
    packet[idx++] = 0x00;
    packet[idx++] = 0x00;
    packet[idx++] = 0x00;
    packet[idx++] = 0x12;
  }
  else {
    Serial.println("Unknown command");
    return -1;
  }

  clearSerialBuffer(_serial);
  vTaskDelay(pdMS_TO_TICKS(10));

  _serial.write(packet, 8);
  
  _serial.flush();
  vTaskDelay(pdMS_TO_TICKS(20));

  Serial.printf("Sent Command: %s\n", command.c_str());

  int result = readOut();
  return result;
};
