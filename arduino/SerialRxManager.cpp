#include "SerialRxManager.h"

#define SERIAL Serial1

SerialRxManager::SerialRxManager() {}

void SerialRxManager::init() {
    SERIAL.begin(baudRate); // Serial1 is for uart. Serial is for usb
    inputBuffer.reserve(64);
    _running = true;
}

// read data from serial port
void SerialRxManager::update(unsigned long now) {
    if (!_running) return;

    // if there is serial input, read it and store it into buffer
    while (SERIAL.available() > 0) {
        char c = SERIAL.read();
        if (c == '\n') {
            if ( dataReceivedCallback && inputBuffer.length() > 0 ) dataReceivedCallback(inputBuffer);
            inputBuffer = "";
        } else if (c != '\r') {
            inputBuffer += c;
        }
    }
}

void SerialRxManager::setBaudRate(unsigned long baudRate) {
    this->baudRate = baudRate;
    this->_running = false;
    SERIAL.end();
    delay(10);
    SERIAL.begin(baudRate);
    this->_running = true;
}

unsigned long SerialRxManager::getBaudRate() const {
    return baudRate;
}

// process every received data
// will call this function in stateManager.init()
// in 
void SerialRxManager::attachDataReceivedCallback(void (*cb)(const String& data)) {
    dataReceivedCallback = cb;
}