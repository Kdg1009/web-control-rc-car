#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "BasicManager.h"
#include "DisplayManager.h"
#include "MotorManager.h"
#include "ServoManager.h"
#include "SerialRxManager.h"

enum BootStep {
  BOOT_START = 0,
  BOOT_WIFI_CONNECTING,
  BOOT_WIFI_CONNECTED,
  BOOT_WIFI_GOT_IP,
  BOOT_WEBSERVER_START,
  BOOT_READY,
  INVALID_CMD
};

class StateManager {
  public:
    static StateManager& instance();

    void init();
    void update(unsigned long now);

    BootStep getBootStep() const;
    String getIPAddress() const;
    void cmd_serial(const String& data); // String input -> call all cmd functions

  private:
    StateManager();

    DisplayManager display;
    MotorManager motor;
    ServoManager servo;
    SerialRxManager serialRx;

    bool wifi_connected = false;
    String ipAddr;
    BootStep bootStep = BOOT_START;
    unsigned long lastUpdateMs = 0;

    void setBootStep(BootStep s);
    void setIPAddress(const String ipAddr);

    void cmd_setMotorSpeed(uint8_t rate);
    void cmd_setMotorDir(int dir);
    void cmd_setSteering(int angle);

    void handleControlMsg(const String& msg);
    void handleIPMsg(const String& msg);
    void handleServerStartMsg(const String& msg);
    void handleConnectionLostMsg(const String& msg);
};

#endif
