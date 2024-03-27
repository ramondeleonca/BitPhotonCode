#include <Arduino.h>
#include <WiFi.h>
#include <SerialCommand.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <state.cpp>
#include <utils.cpp>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <BH1750.h>
#include <Wire.h>

// Global variables
int deviceId = 0;

// Constants
const String PRODUCT_NAME = "BitPhoton";
const String PRODUCT_VERSION = "1.0.0";
const String PRODUCT_ID = PRODUCT_NAME + "@" + PRODUCT_VERSION;

const int SERVER_PORT = 80;
const int SOCKET_PORT = 81;

const int UPDATE_INTERVAL = 200;

//! FOR ESP32
const int LED_PINS[4] = { 5, 17, 16, 4 };

//! FOR ESP32 S3
// const int LED_PINS[4] = { 36, 37, 38, 39 };

// Preferences
Preferences preferences;

// Servers
AsyncWebServer server(SERVER_PORT);
WebSocketsServer socket(SOCKET_PORT);

// Commands
SerialCommand commands;

// State
DeviceState state;

// Sensors
BH1750 lightSensor;

class wifi {
    public:
        static wl_status_t connect() {
            String ssid = preferences.getString("wifi/ssid");
            String password = preferences.getString("wifi/password");

            if (ssid.length() > 0) {
                Serial.println("Attempting to connect to wifi with ssid: " + ssid);
                WiFi.disconnect();
                WiFi.begin(ssid.c_str(), password.length() > 0 ? password.c_str() : NULL);
                wl_status_t result = (wl_status_t)WiFi.waitForConnectResult();
                delay(100);
                if (result == WL_CONNECTED) {
                    Serial.println("Successfully connected to wifi");
                    Serial.println("WIFI/IP: " + WiFi.localIP().toString());
                    Serial.println("WIFI/MAC: " + WiFi.macAddress());
                } else {
                    Serial.println("Failed to connect to wifi");
                }
                return result;
            } else {
                Serial.println("No wifi credentials found");
                return WL_IDLE_STATUS;
            }
        }

        static void disconnect() {
            if (WiFi.disconnect()) {
                Serial.println("Disconnected from wifi");
            } else {
                Serial.println("Failed to disconnect from wifi");
            }
        }

        static void scan() {
            int networks = WiFi.scanNetworks();
            Serial.println("Found " + String(networks) + " networks");
            for (int i = 0; i < networks; i++) Serial.println("SSID: " + WiFi.SSID(i) + " | " + "RSSI: " + WiFi.RSSI(i));
        }
};

class ap {
    public:
        static bool start() {
            bool result = WiFi.softAP(String(PRODUCT_NAME + "-" + deviceId).c_str());
            Serial.println("AP/IP: " + WiFi.softAPIP().toString());
            Serial.println("AP/MAC: " + WiFi.softAPmacAddress());
            return result;
        }

        static void stop() {
            WiFi.softAPdisconnect();
        }
};

class mdns {
    public:
        static void start() {
            if (MDNS.begin(String(PRODUCT_NAME + "-" + deviceId).c_str())) {
                MDNS.addService("http", "tcp", SERVER_PORT);
                MDNS.addService("ws", "tcp", SOCKET_PORT);
                Serial.println("MDNS started");
                Serial.println("MDNS/NAME: " + String(PRODUCT_NAME + "-" + deviceId + ".local"));
            } else {
                Serial.println("MDNS failed to start");
            }
        }

        static void stop() {
            MDNS.end();
        }
};

class Commands {
    public:
        static void config() {
            char* action = commands.next();
            char* key = commands.next();
            char* value = commands.next();

            if (action == NULL) {
                Serial.println("Invalid command");
                return;
            }

            if (strcmp(action, "set") == 0) {
                preferences.putString(key, value);
                Serial.println("Set " + String(key) + " to " + String(value));
            } else if (strcmp(action, "get") == 0) {
                Serial.println(preferences.getString(key));
            } else if (strcmp(action, "clear") == 0) {
                preferences.clear();
                Serial.println("Cleared all preferences");
            } else {
                Serial.println("Invalid action");
            }
        }

        static void wifi() {
            char* action = commands.next();

            if (action == NULL) {
                Serial.println("Invalid command");
                return;
            }

            if (strcmp(action, "connect") == 0) {
                wifi::connect();
                server.begin();
                socket.begin();
                mdns::start();
            } else if (strcmp(action, "disconnect") == 0) {
                wifi::disconnect();
            } else if (strcmp(action, "scan") == 0) {
                wifi::scan();
            } else {
                Serial.println("Invalid action");
            }
        }

        static void ap() {
            char* action = commands.next();

            if (action == NULL) {
                Serial.println("Invalid command");
                return;
            }

            if (strcmp(action, "start") == 0) {
                ap::start();
                server.begin();
                socket.begin();
                mdns::start();
            } else if (strcmp(action, "stop") == 0) {
                ap::stop();
            } else {
                Serial.println("Invalid action");
            }
        }

        static void get_ip() {
            Serial.println(WiFi.localIP().toString());
        }

        static void get_mac() {
            Serial.println(WiFi.macAddress());
        }

        static void restart() {
            ESP.restart();
        }

        static void get_state() {
            Serial.println(state.serialize());
        }
};

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = socket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            break;
    }
}

void setup() {
    // Start serial
    Serial.begin(115200);

    // Start preferences
    preferences.begin(PRODUCT_NAME.c_str(), false);

    // Load persistent values
    try {
        String id =  preferences.getString("id");
        if (id.length() > 0) deviceId = id.toInt();
        else throw std::runtime_error("No device id found");
    } catch (const std::exception& e) {
        Serial.println("An error occurred: " + String(e.what()));
        Serial.println("Resetting device id to 0");
        preferences.putString("id", "0");
        deviceId = 0;
    }

    // Configure all leds
    for (int i = 0; i < 4; i++) {
        Serial.println("Configuring pin " + String(LED_PINS[i]));
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }

    // Write device id to leds
    int leds[4];
    utils::getLEDDigit(deviceId, leds);
    for (int i = 0; i < 4; i++) digitalWrite(LED_PINS[i], leds[i]);
    if (deviceId == 0) analogWrite(LED_BUILTIN, 128);
    else digitalWrite(LED_BUILTIN, LOW);

    // Start device functionality
    Wire.begin();
    lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    
    // Configure state
    state.diskUsage = (float)ESP.getSketchSize() / (float)ESP.getFlashChipSize();

    // Configure wifi
    WiFi.setAutoReconnect(true);
    WiFi.useStaticBuffers(true);
    WiFi.setMinSecurity(WIFI_AUTH_OPEN);

    // Start wifi
    try {
        if (wifi::connect() != WL_CONNECTED) throw std::runtime_error("Failed to connect to wifi");
    } catch (const std::exception& e) {
        Serial.println("An error occurred: " + String(e.what()));
        Serial.println("Starting AP mode");
        ap::start();
    }
    if (preferences.getString("ap/enable") == "true") ap::start();
    server.begin();
    socket.begin();
    mdns::start();

    // Register websocket event
    socket.onEvent(onWebSocketEvent);

    // Configure server
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Register routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", PRODUCT_ID);
    });

    server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", state.serialize());
    });

    server.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;

        doc["id"] = deviceId;
        doc["product"] = PRODUCT_NAME;
        doc["version"] = PRODUCT_VERSION;
        doc["wifiMode"] = WiFi.getMode() == WIFI_AP ? "ap" : "client";
        doc["wifiSSID"] = WiFi.SSID();
        doc["wifiIP"] = WiFi.localIP().toString();
        doc["wifiMAC"] = WiFi.macAddress();
        doc["apSSID"] = WiFi.softAPSSID();
        doc["apIP"] = WiFi.softAPIP().toString();
        doc["apMAC"] = WiFi.softAPmacAddress();
        doc["mdnsName"] = String(PRODUCT_NAME + "-" + deviceId + ".local");
        doc["chip"] = ESP.getChipModel();
        doc["chipCores"] = ESP.getChipCores();
        doc["flashSize"] = ESP.getFlashChipSize();
        doc["memoryUsage"] = (float)ESP.getFreeHeap() / (float)ESP.getHeapSize();
        doc["diskUsage"] = (float)ESP.getSketchSize() / (float)ESP.getFlashChipSize();
        doc["uptime"] = (int)millis() / 1000;

        String result;
        serializeJson(doc, result);
        request->send(200, "application/json", result);
    });

    server.on("/config/set", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("key", true) && request->hasParam("value", true)) {
            String key = request->getParam("key", true)->value();
            String value = request->getParam("value", true)->value();
            preferences.putString(key.c_str(), value.c_str());
            request->send(200, "text/plain", "Set " + key + " to " + value);
        } else {
            request->send(400, "text/plain", "Invalid request");
        }
    });

    server.on("/config/get", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("key", true)) {
            String key = request->getParam("key", true)->value();
            request->send(200, "text/plain", preferences.getString(key.c_str()));
        } else {
            request->send(400, "text/plain", "Invalid request");
        }
    });

    server.on("/config/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
        preferences.clear();
        request->send(200, "text/plain", "Cleared all preferences");
    });

    server.on("/wifi/connect", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (wifi::connect() == WL_CONNECTED) {
            request->send(200, "text/plain", "Connected to wifi");
        } else {
            request->send(500, "text/plain", "Failed to connect to wifi");
        }
    });

    server.on("/wifi/disconnect", HTTP_GET, [](AsyncWebServerRequest *request) {
        wifi::disconnect();
        request->send(200, "text/plain", "Disconnected from wifi");
    });

    server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int networks = WiFi.scanNetworks();
        String result = "Found " + String(networks) + " networks\n";
        for (int i = 0; i < networks; i++) result += "SSID: " + WiFi.SSID(i) + " | " + "RSSI: " + WiFi.RSSI(i) + "\n";
        request->send(200, "text/plain", result);
    });

    server.on("/ap/start", HTTP_GET, [](AsyncWebServerRequest *request) {
        ap::start();
        request->send(200, "text/plain", "Started AP mode");
    });

    server.on("/ap/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
        ap::stop();
        request->send(200, "text/plain", "Stopped AP mode");
    });

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Restarting device...");
        ESP.restart();
    });

    // Register commands
    commands.addCommand("config", Commands::config);
    commands.addCommand("wifi", Commands::wifi);
    commands.addCommand("ap", Commands::ap);
    commands.addCommand("get_ip", Commands::get_ip);
    commands.addCommand("get_mac", Commands::get_mac);
    commands.addCommand("restart", Commands::restart);
    commands.addCommand("get_state", Commands::get_state);

    // Print device info
    Serial.println("DEVICE/ID: " + String(deviceId));
    Serial.println("PRODUCT/ID: " + PRODUCT_ID);
    Serial.println("PRODUCT/NAME: " + PRODUCT_NAME);
    Serial.println("PRODUCT/VERSION: " + PRODUCT_VERSION);
}

unsigned long lastTime = 0;
unsigned long lastUpdate = 0;
void loop() {
    // Update time
    unsigned long currentTime = millis();
    int deltaTime = (int)currentTime - (int)lastTime;

    // Update state
    if (currentTime - lastUpdate > UPDATE_INTERVAL) {
        state.memoryUsage = (float)ESP.getFreeHeap() / (float)ESP.getHeapSize();
        state.deltaTime = (int)currentTime - (int)lastUpdate;
        state.uptime = (int)currentTime / 1000;

        int light = lightSensor.readLightLevel();
        state.light = light;

        if (light < 40) {
            state.lightEnum = "dark";
        } else if (light < 100) {
            state.lightEnum = "dim";
        } else if (light < 200) {
            state.lightEnum = "normal";
        } else {
            state.lightEnum = "bright";
        }

        // Send state to clients
        String serializedState = state.serialize();
        socket.broadcastTXT(serializedState.c_str(), serializedState.length());

        lastUpdate = currentTime;
    }

    // Update socket
    socket.loop();

    // Process commands
    commands.readSerial();
    commands.clearBuffer();
    
    // Update time
    lastTime = currentTime;
}
