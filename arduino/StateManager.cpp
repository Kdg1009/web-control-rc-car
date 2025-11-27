#include "StateManager.h"

StateManager& StateManager::instance() {
  static StateManager inst;
  return inst;
}

StateManager::StateManager() {}

void StateManager::init() {
  // 1. init display & serial_rx
  display.init();
  motor.init();
  servo.init();
  serialRx.init();
  serialRx.attachDataReceivedCallback(
    [](const String& msg) {
      StateManager::instance().cmd_serial(msg);
    }
  );

  setBootStep(BOOT_START);
  lastUpdateMs = millis();
}

void StateManager::update(unsigned long now) {
  serialRx.update(now);
  motor.update(now);
  servo.update(now);
  display.update(now);

  display.setInfo(motor.getMaxOutput(), motor.getDirection(), servo.getAngle());
  lastUpdateMs = now;
}

BootStep StateManager::getBootStep() const { return bootStep; }
String StateManager::getIPAddress() const { 
  if (wifi_connected) {
    return ipAddr;
  } else {
    return "0.0.0.0";
  }
}

// command from webserver
void StateManager::cmd_serial(const String& msg) {
  int spaceIndex = msg.indexOf(' ');
  String head = (spaceIndex == -1) ? msg : msg.substring(0, spaceIndex);
  // 1. update motor, servo and display
  if (head == "SPD") {
    handleControlMsg(msg);
  }
  // 2. update ip addr
  else if (head == "IP") {
    handleIPMsg(msg);
  }
  // 3. server started
  else if (head == "SERVER") {
    handleServerStartMsg(msg);
  }
  // 4. connection lost
  else if (head == "WIFI") {
    handleConnectionLostMsg(msg);
  }
  else {
    setBootStep(INVALID_CMD);
    display.setInvalidMsg(msg);
  }
}

void StateManager::setIPAddress(const String ipAddr) {
  this->ipAddr = ipAddr;
  display.setIPAddress(ipAddr);
}
void StateManager::handleControlMsg(const String& msg) {
  if (getBootStep() != BOOT_READY) {
    setBootStep(BOOT_READY);
  }
  int speed = 0;
  int dir = 0;
  int steer = 0;

  sscanf(msg.c_str(), "SPD %d DIR %d STEER %d", &speed, &dir, &steer);

  cmd_setMotorSpeed((uint8_t)speed);
  cmd_setMotorDir(dir);
  cmd_setSteering(steer);
}

void StateManager::handleIPMsg(const String& msg) {
  if (!wifi_connected) {
    int spaceIndex = msg.indexOf(' ');
    if (spaceIndex < 0) return;

    String ip = msg.substring(spaceIndex + 1);
    setIPAddress(ip);
    
    wifi_connected = true;
    setBootStep(BOOT_WIFI_GOT_IP);
  }
}

void StateManager::handleServerStartMsg(const String& msg) {
  if (wifi_connected) {
    setBootStep(BOOT_READY);
  }
}

void StateManager::handleConnectionLostMsg(const String& msg) {
  wifi_connected = false;
  setBootStep(BOOT_WIFI_CONNECTING);
}

void StateManager::cmd_setMotorSpeed(uint8_t rate) {
  motor.setMaxOutput(rate);
}

void StateManager::cmd_setMotorDir(int dir) {
  if (dir == 0) {
    motor.setDirection(MotorManager::FORWARD);
  } else if (dir == 1) {
    motor.setDirection(MotorManager::BACKWARD);
  } else if (dir == 2) {
    motor.setDirection(MotorManager::STOP);
  }
}

void StateManager::cmd_setSteering(int angle) {
  if (angle <= ServoManager::LEFT) {
    servo.setAngle(ServoManager::LEFT);
  } else if (ServoManager::RIGHT <= angle) {
    servo.setAngle(ServoManager::RIGHT);
  } else {
    servo.setAngle(ServoManager::STR);
  }
}

void StateManager::setBootStep(BootStep s) {
  bootStep = s;

  switch (s) {
    case BOOT_START:
      display.setStat(DisplayManager::BOOT_START);
      break;

    case BOOT_WIFI_CONNECTING:
      display.setStat(DisplayManager::WIFI_CONNECTING);
      break;

    case BOOT_WIFI_CONNECTED:
      display.setStat(DisplayManager::WIFI_CONNECTED);
      break;

    case BOOT_WIFI_GOT_IP:
      display.setStat(DisplayManager::WIFI_GOT_IP);
      break;

    case BOOT_WEBSERVER_START:
      display.setStat(DisplayManager::WEBSERVER_START);
      break;

    case BOOT_READY:
      display.setStat(DisplayManager::WEBSERVER_READY);
      break;
    case INVALID_CMD:
      display.setStat(DisplayManager::INVALID_CMD);
  }
  display.update(0);
}