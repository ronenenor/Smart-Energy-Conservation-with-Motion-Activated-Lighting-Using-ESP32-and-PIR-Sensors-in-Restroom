#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

// WiFi credentials
const char *ssid = "ssid";
const char *password = "psswd";

// Static IP configuration
IPAddress staticIP(x, 1x, x, x);     // The static IP address assigned to ESP32
IPAddress gateway(x, x, x, 1);       // The gateway (router) IP address - NEEDS VERIFICATION
IPAddress subnet(255, 255, 252, 0);        // The subnet mask - NEEDS VERIFICATION
IPAddress dns1(8, 8, 8, 8);              // Primary DNS
IPAddress dns2(8, 8, 4, 4);              // Secondary DNS

// Define pins for sensors and relays
#define SENSOR_PIN_1 25  // First PIR sensor connected to GPIO25
#define SENSOR_PIN_2 33  // Second PIR sensor connected to GPIO33
#define RELAY_PIN_1 26   // First relay channel connected to GPIO26
#define RELAY_PIN_2 27   // Second relay channel connected to GPIO27
#define RELAY_PIN_3 14   // Third relay channel connected to GPIO14

// Timer variables
unsigned long lastMotionDetectedTime = 0;
unsigned long motionCheckInterval = 100;     // Check sensor every 100ms
unsigned long motionCooldownPeriod = 90000;  // 90 seconds cooldown (changed from 5000)

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Variables to store sensor and relay states
bool relay1State = false;
bool relay2State = false;
bool relay3State = false;
bool sensor1State = false;
bool sensor2State = false;
bool manualOverride1 = false;
bool manualOverride2 = false;
bool manualOverride3 = false;

// Function to initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

// Function to initialize WiFi with static IP
void initWiFi() {
  WiFi.mode(WIFI_STA);
  
  // Configure static IP
  if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
    Serial.println("Failed to configure static IP");
  }
  
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to send current states to all connected clients
void notifyClients() {
  String json = "{";
  json += "\"relay1\":" + String(relay1State ? "true" : "false") + ",";
  json += "\"relay2\":" + String(relay2State ? "true" : "false") + ",";
  json += "\"relay3\":" + String(relay3State ? "true" : "false") + ",";
  json += "\"sensor1\":" + String(sensor1State ? "true" : "false") + ",";
  json += "\"sensor2\":" + String(sensor2State ? "true" : "false") + ",";
  json += "\"override1\":" + String(manualOverride1 ? "true" : "false") + ",";
  json += "\"override2\":" + String(manualOverride2 ? "true" : "false") + ",";
  json += "\"override3\":" + String(manualOverride3 ? "true" : "false") + ",";
  json += "\"cooldownTime\":" + String(motionCooldownPeriod / 1000);  // Add current cooldown time in seconds
  json += "}";
  ws.textAll(json);
}

// Handle WebSocket messages
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char *)data;

    if (message.indexOf("toggle1") >= 0) {
      manualOverride1 = !manualOverride1;
      if (manualOverride1) {
        relay1State = !relay1State;
        digitalWrite(RELAY_PIN_1, relay1State ? LOW : HIGH);
      }
      notifyClients();
    } else if (message.indexOf("toggle2") >= 0) {
      manualOverride2 = !manualOverride2;
      if (manualOverride2) {
        relay2State = !relay2State;
        digitalWrite(RELAY_PIN_2, relay2State ? LOW : HIGH);
      }
      notifyClients();
    } else if (message.indexOf("toggle3") >= 0) {
      manualOverride3 = !manualOverride3;
      if (manualOverride3) {
        relay3State = !relay3State;
        digitalWrite(RELAY_PIN_3, relay3State ? LOW : HIGH);
      }
      notifyClients();
    } else if (message.indexOf("auto1") >= 0) {
      manualOverride1 = false;
      notifyClients();
    } else if (message.indexOf("auto2") >= 0) {
      manualOverride2 = false;
      notifyClients();
    } else if (message.indexOf("auto3") >= 0) {
      manualOverride3 = false;
      notifyClients();
    }
    // Add handling for the "allOn" command
    else if (message.indexOf("allOn") >= 0) {
      // Turn on all relays and set to manual mode
      manualOverride1 = true;
      manualOverride2 = true;
      manualOverride3 = true;
      relay1State = true;
      relay2State = true;
      relay3State = true;
      digitalWrite(RELAY_PIN_1, LOW);  // LOW turns on relay
      digitalWrite(RELAY_PIN_2, LOW);  // LOW turns on relay
      digitalWrite(RELAY_PIN_3, LOW);  // LOW turns on relay
      notifyClients();
    }
    // Add handling for the "allOff" command
    else if (message.indexOf("allOff") >= 0) {
      // Turn off all relays and set to manual mode
      manualOverride1 = true;
      manualOverride2 = true;
      manualOverride3 = true;
      relay1State = false;
      relay2State = false;
      relay3State = false;
      digitalWrite(RELAY_PIN_1, HIGH);  // HIGH turns off relay
      digitalWrite(RELAY_PIN_2, HIGH);  // HIGH turns off relay
      digitalWrite(RELAY_PIN_3, HIGH);  // HIGH turns off relay
      notifyClients();
    } else if (message.indexOf("setCooldown") >= 0) {
      // Extract cooldown value from message
      int startPos = message.indexOf("=") + 1;
      int newCooldown = message.substring(startPos).toInt();
      if (newCooldown > 0) {
        motionCooldownPeriod = newCooldown * 1000;  // Convert to milliseconds
        notifyClients();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Send current states to newly connected client
      notifyClients();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize pins
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);
  pinMode(SENSOR_PIN_1, INPUT);
  pinMode(SENSOR_PIN_2, INPUT);

  // Ensure relays are off initially (HIGH for low-level triggered relays)
  digitalWrite(RELAY_PIN_1, HIGH);
  digitalWrite(RELAY_PIN_2, HIGH);
  digitalWrite(RELAY_PIN_3, HIGH);

  // Initialize file system and WiFi
  initFS();
  initWiFi();
  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Route for style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css", "text/css");
  });

  // Route for script.js
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "text/javascript");
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();

  // Read sensor values
  int sensor1Value = digitalRead(SENSOR_PIN_1);
  int sensor2Value = digitalRead(SENSOR_PIN_2);

  // Update sensor states
  bool prevSensor1State = sensor1State;
  bool prevSensor2State = sensor2State;
  sensor1State = (sensor1Value == HIGH);
  sensor2State = (sensor2Value == HIGH);

  // Check if motion is detected by EITHER sensor
  bool motionDetected = sensor1State || sensor2State;

  // Handle motion detection for all three relays
  if (motionDetected) {
    // Motion is currently detected - update the timestamp
    lastMotionDetectedTime = millis();

    // Handle Relay 1
    if (!manualOverride1 && !relay1State) {
      relay1State = true;
      digitalWrite(RELAY_PIN_1, LOW);  // LOW turns on the relay
      Serial.println("Motion detected - Turning ON relay 1");
    }

    // Handle Relay 2
    if (!manualOverride2 && !relay2State) {
      relay2State = true;
      digitalWrite(RELAY_PIN_2, LOW);  // LOW turns on the relay
      Serial.println("Motion detected - Turning ON relay 2");
    }

    // Handle Relay 3
    if (!manualOverride3 && !relay3State) {
      relay3State = true;
      digitalWrite(RELAY_PIN_3, LOW);  // LOW turns on the relay
      Serial.println("Motion detected - Turning ON relay 3");
    }

    // If any relay state changed, notify clients
    if (!relay1State || !relay2State || !relay3State) {
      notifyClients();
    }
  } else {
    // No motion currently detected by either sensor
    // Only turn off if enough time has passed since last motion
    if (millis() - lastMotionDetectedTime > motionCooldownPeriod) {
      bool stateChanged = false;

      // Handle Relay 1
      if (!manualOverride1 && relay1State) {
        relay1State = false;
        digitalWrite(RELAY_PIN_1, HIGH);  // HIGH turns off the relay
        Serial.println("No motion detected for the cooldown period - Turning OFF relay 1");
        stateChanged = true;
      }

      // Handle Relay 2
      if (!manualOverride2 && relay2State) {
        relay2State = false;
        digitalWrite(RELAY_PIN_2, HIGH);  // HIGH turns off the relay
        Serial.println("No motion detected for the cooldown period - Turning OFF relay 2");
        stateChanged = true;
      }

      // Handle Relay 3
      if (!manualOverride3 && relay3State) {
        relay3State = false;
        digitalWrite(RELAY_PIN_3, HIGH);  // HIGH turns off the relay
        Serial.println("No motion detected for the cooldown period - Turning OFF relay 3");
        stateChanged = true;
      }

      // If any relay state changed, notify clients
      if (stateChanged) {
        notifyClients();
      }
    }
  }

  // Notify clients if any sensor state changed
  if (prevSensor1State != sensor1State || prevSensor2State != sensor2State) {
    notifyClients();
  }

  delay(motionCheckInterval);  // Check for motion at regular intervals
}