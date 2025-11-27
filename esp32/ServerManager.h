#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "CameraManager.h"

class ServerManager {
public:
    ServerManager();
    void init();
    void update(unsigned long now);

private:
    void handleRoot();
    void handleStreamRequest();
    void handleControlMsg();
    void handleIPMsg() const;
    void handleServerStartMsg();
    void handleWiFiLostMsg();

    struct StreamClient {
        WiFiClient client;
        bool active = false;
        unsigned long lastFrameTime = 0;
    };

    bool wifi_connected = false;
    bool _running = false;

    WebServer server;
    CameraManager cam;

    StreamClient streamClient;

    unsigned long lastControlMsgTime = 0;
    unsigned long lastIPMsgTime = 0;

    const unsigned long streamInterval = 120;     // ~8 FPS
    const unsigned long controlMsInterval = 100; // 10 FPS
    const unsigned long ipMsInterval = 5000;     // 0.2 FPS

    int speed = 0;
    int dir = 0;
    int steer = 105;
};

extern const char* index_html;

#endif
