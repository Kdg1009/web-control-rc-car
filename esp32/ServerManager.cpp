#include "ServerManager.h"
#include "secret.h"

#ifdef STA
void wifiInit(bool& wifi_connected) {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int trials = 0;

  while (WiFi.status() != WL_CONNECTED && trials < 10) {
    delay(300);
    ++trials;
  }

  wifi_connected = (WiFi.status() == WL_CONNECTED);
}
#endif
#ifdef AP
void wifiInit(bool& wifi_connected) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address(AP): ");
  Serial.print(IP);

  wifi_connected = true;
}
#endif

ServerManager::ServerManager() : server(80) {}

void ServerManager::init() {
    Serial.begin(115200); // esp32 doesn't support serial1

    wifi_connected = (WiFi.status() == WL_CONNECTED);
    wifiInit(wifi_connected);
    handleIPMsg();

    cam.init();

    server.on("/", HTTP_GET, [this]() { handleRoot(); });

    // Set only request; actual frame sending happens in update()
    server.on("/stream", HTTP_GET, [this]() { handleStreamRequest(); });

    server.on("/control", HTTP_GET, [this]() { handleControlMsg(); });
    
    server.begin();
    handleServerStartMsg();

    _running = true;
}

void ServerManager::update(unsigned long now) {
    if (!_running) return;

    server.handleClient();
    //newly added
    yield(); // allow wifi to breath
    //newly added

    // Stream MJPEG
    if (streamClient.active) {
        if (!streamClient.client.connected()) {
            streamClient.client.stop();
            streamClient.active = false;
            return;
        }

        if (now - streamClient.lastFrameTime > streamInterval) {
            camera_fb_t* fb = esp_camera_fb_get();
            if (!fb) {
              //newly added
              yield();
              //newly added
              return;
            }

            streamClient.client.printf("--frame\r\n");
            streamClient.client.printf("Content-Type: image/jpeg\r\n");
            streamClient.client.printf("Content-Length: %u\r\n\r\n", fb->len);
            streamClient.client.write(fb->buf, fb->len);
            streamClient.client.printf("\r\n");

            esp_camera_fb_return(fb);
            streamClient.lastFrameTime = now;

            // newly added
            yield(); // avoid WDT and socket starvation
            // newly added
        }
    }

    // periodic IP print
    if (now - lastIPMsgTime > ipMsInterval) {
        lastIPMsgTime = now;
        handleIPMsg();
    }
}

void ServerManager::handleRoot() {
    server.send(200, "text/html", index_html);
}

void ServerManager::handleStreamRequest() {
    WiFiClient client = server.client();
    if (!client) return;

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    // newly added
    client.println("Cache-Control: no-cache");
    client.println("Pragma: no-cache");
    // newly added
    client.println("Connection: close");
    client.println();

    streamClient.client = client;
    streamClient.active = true;
    streamClient.lastFrameTime = millis();
}

void ServerManager::handleControlMsg() {
    if (server.hasArg("speed") &&
        server.hasArg("dir") &&
        server.hasArg("steer")) {

        speed = server.arg("speed").toInt();
        dir = server.arg("dir").toInt();
        steer = server.arg("steer").toInt();

        Serial.printf("SPD %d DIR %d STEER %d\n", speed, dir, steer);

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void ServerManager::handleIPMsg() const {
    if (!wifi_connected) return;
    Serial.print("IP ");
    Serial.println(WiFi.localIP());
}

void ServerManager::handleServerStartMsg() {
    Serial.println("SERVER START");
}

void ServerManager::handleWiFiLostMsg() {
    Serial.println("WIFI LOST");
}

// HTML page with embedded JavaScript
const char* index_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-CAM RC Car Control</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      font-family: Arial, sans-serif;
      background-color: #1a1a1a;
      color: #ffffff;
      overflow: hidden;
    }
    .container {
      display: flex;
      height: 100vh;
    }
    .camera-section {
      flex: 1;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      background-color: #000;
      padding: 20px;
    }
    #stream {
      max-width: 100%;
      max-height: 100%;
      border: 2px solid #333;
      border-radius: 8px;
    }
    .control-section {
      width: 400px;
      background-color: #2a2a2a;
      padding: 30px;
      display: flex;
      flex-direction: column;
      gap: 30px;
    }
    h1 {
      text-align: center;
      color: #4CAF50;
      margin-bottom: 10px;
    }
    .speed-control {
      background-color: #333;
      padding: 20px;
      border-radius: 8px;
    }
    .speed-control h2 {
      font-size: 18px;
      margin-bottom: 15px;
      color: #4CAF50;
    }
    .slider-container {
      display: flex;
      align-items: center;
      gap: 15px;
    }
    input[type="range"] {
      flex: 1;
      height: 8px;
      border-radius: 5px;
      background: #555;
      outline: none;
      -webkit-appearance: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
    }
    input[type="range"]::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
      border: none;
    }
    #speedValue {
      min-width: 50px;
      text-align: center;
      font-size: 18px;
      font-weight: bold;
      color: #4CAF50;
    }
    .direction-control {
      background-color: #333;
      padding: 20px;
      border-radius: 8px;
    }
    .direction-control h2 {
      font-size: 18px;
      margin-bottom: 15px;
      color: #4CAF50;
      text-align: center;
    }
    .arrow-grid {
      display: grid;
      grid-template-columns: repeat(3, 80px);
      grid-template-rows: repeat(3, 80px);
      gap: 10px;
      justify-content: center;
    }
    .arrow-btn {
      background-color: #444;
      border: 2px solid #555;
      border-radius: 8px;
      color: white;
      font-size: 24px;
      cursor: pointer;
      transition: all 0.1s;
      display: flex;
      align-items: center;
      justify-content: center;
      user-select: none;
    }
    .arrow-btn:active, .arrow-btn.pressed {
      background-color: #4CAF50;
      border-color: #45a049;
      transform: scale(0.95);
    }
    .arrow-btn:hover {
      background-color: #555;
    }
    .arrow-up { grid-column: 2; grid-row: 1; }
    .arrow-down { grid-column: 2; grid-row: 3; }
    .arrow-left { grid-column: 1; grid-row: 2; }
    .arrow-right { grid-column: 3; grid-row: 2; }
    .status {
      background-color: #333;
      padding: 15px;
      border-radius: 8px;
      text-align: center;
      font-size: 14px;
    }
    .status-indicator {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      margin-right: 8px;
      background-color: #4CAF50;
    }
    @media (max-width: 768px) {
      .container {
        flex-direction: column;
      }
      .control-section {
        width: 100%;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="camera-section">
      <img id="stream" src="/stream">
    </div>
    <div class="control-section">
      <h1>RC Car Control</h1>
      
      <div class="speed-control">
        <h2>Speed Control</h2>
        <div class="slider-container">
          <input type="range" id="speedSlider" min="0" max="255" value="0">
          <span id="speedValue">0</span>
        </div>
      </div>
      
      <div class="direction-control">
        <h2>Direction Control</h2>
        <div class="arrow-grid">
          <button class="arrow-btn arrow-up" id="btnUp">▲</button>
          <button class="arrow-btn arrow-down" id="btnDown">▼</button>
          <button class="arrow-btn arrow-left" id="btnLeft">◄</button>
          <button class="arrow-btn arrow-right" id="btnRight">►</button>
        </div>
      </div>
      
      <div class="status">
        <span class="status-indicator"></span>
        <span>Connected</span>
      </div>
    </div>
  </div>

  <script>
    let currentSpeed = 0;
    let upPressed = false;
    let downPressed = false;
    let leftPressed = false;
    let rightPressed = false;

    // UI Elements
    const speedSlider = document.getElementById('speedSlider');
    const speedValue = document.getElementById('speedValue');
    const btnUp = document.getElementById('btnUp');
    const btnDown = document.getElementById('btnDown');
    const btnLeft = document.getElementById('btnLeft');
    const btnRight = document.getElementById('btnRight');
    
    // Speed control
    speedSlider.addEventListener('input', function() {
      currentSpeed = this.value;
      speedValue.textContent = currentSpeed;
      sendCommand();
    });

    // Button event listeners
    function addButtonListeners(btn, pressCallback, releaseCallback) {
      btn.addEventListener('mousedown', pressCallback);
      btn.addEventListener('mouseup', releaseCallback);
      btn.addEventListener('mouseleave', releaseCallback);
      btn.addEventListener('touchstart', (e) => { e.preventDefault(); pressCallback(); });
      btn.addEventListener('touchend', (e) => { e.preventDefault(); releaseCallback(); });
    }

    addButtonListeners(btnUp, 
      () => { upPressed = true; btnUp.classList.add('pressed'); sendCommand(); },
      () => { upPressed = false; btnUp.classList.remove('pressed'); sendCommand(); }
    );

    addButtonListeners(btnDown,
      () => { downPressed = true; btnDown.classList.add('pressed'); sendCommand(); },
      () => { downPressed = false; btnDown.classList.remove('pressed'); sendCommand(); }
    );

    addButtonListeners(btnLeft,
      () => { leftPressed = true; btnLeft.classList.add('pressed'); sendCommand(); },
      () => { leftPressed = false; btnLeft.classList.remove('pressed'); sendCommand(); }
    );

    addButtonListeners(btnRight,
      () => { rightPressed = true; btnRight.classList.add('pressed'); sendCommand(); },
      () => { rightPressed = false; btnRight.classList.remove('pressed'); sendCommand(); }
    );

    // Keyboard control
    document.addEventListener('keydown', function(e) {
      if (e.key === 'ArrowUp' && !upPressed) {
        upPressed = true;
        btnUp.classList.add('pressed');
        sendCommand();
      } else if (e.key === 'ArrowDown' && !downPressed) {
        downPressed = true;
        btnDown.classList.add('pressed');
        sendCommand();
      } else if (e.key === 'ArrowLeft' && !leftPressed) {
        leftPressed = true;
        btnLeft.classList.add('pressed');
        sendCommand();
      } else if (e.key === 'ArrowRight' && !rightPressed) {
        rightPressed = true;
        btnRight.classList.add('pressed');
        sendCommand();
      }
    });

    document.addEventListener('keyup', function(e) {
      if (e.key === 'ArrowUp' && upPressed) {
        upPressed = false;
        btnUp.classList.remove('pressed');
        sendCommand();
      } else if (e.key === 'ArrowDown' && downPressed) {
        downPressed = false;
        btnDown.classList.remove('pressed');
        sendCommand();
      } else if (e.key === 'ArrowLeft' && leftPressed) {
        leftPressed = false;
        btnLeft.classList.remove('pressed');
        sendCommand();
      } else if (e.key === 'ArrowRight' && rightPressed) {
        rightPressed = false;
        btnRight.classList.remove('pressed');
        sendCommand();
      }
    });

    function sendCommand() {
      // Determine direction
      let dir = 2; // stop
      if (upPressed) {
        dir = 0; // forward
      } else if (downPressed && !upPressed) {
        dir = 1; // backward
      }

      // Determine steering
      let steer = 105; // straight
      if (leftPressed) {
        steer = 90; // left
      } else if (rightPressed && !leftPressed) {
        steer = 120; // right
      }

      // Send command to server
      fetch(`/control?speed=${currentSpeed}&dir=${dir}&steer=${steer}`)
        .catch(err => console.error('Error:', err));
    }
  setInterval(sendCommand, 50);
  </script>
</body>
</html>
)rawliteral";