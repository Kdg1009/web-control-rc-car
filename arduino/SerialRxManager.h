#ifndef SERIAL_RX_MANAGER_H
#define SERIAL_RX_MANAGER_H

#include <Arduino.h>

class SerialRxManager {
  public:
    SerialRxManager();

    void init();
    void update(unsigned long now); // get msg from server and store it into inputBuffer

    void setBaudRate(unsigned long baudRate);
    unsigned long getBaudRate() const;

    void attachDataReceivedCallback(void (*cb)(const String& data));

  private:
    String inputBuffer = "";
    unsigned long baudRate = 115200;
    bool _running = false;

    void (*dataReceivedCallback)(const String& data) = nullptr;
};

#endif