#ifndef  thx_h
#define  thx_h


#include <Arduino.h>
#include <HardwareSerial.h>

#define RX_PIN 16
#define TX_PIN 17
#define BAUD_RATE 115200

#define START_BYTE 0xEFAA
#define END_BYTE   0x55



class THX_SENSOR{
    public:
        THX_SENSOR(HardwareSerial& serialPort);
        int readOut();
        int sendCommand(std::string command);
    private:
        HardwareSerial& _serial;
};

#endif