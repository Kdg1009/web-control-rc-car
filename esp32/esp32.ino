#include "ServerManager.h"

ServerManager server;

const int LED_PIN = 33;      // ESP32-CAM red status LED
unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    server.init();
}

void loop() {
    server.update(millis());

    // ---- LED heartbeat ----
    unsigned long now = millis();
    if (now - lastBlink >= 500) {   // blink every 500ms
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        lastBlink = now;
    }
}
