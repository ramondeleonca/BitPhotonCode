#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct DeviceState {
    float memoryUsage;
    float diskUsage;
    int deltaTime;
    int uptime;

    int light;
    String lightEnum;

    String serialize() {
        JsonDocument doc;

        doc["memoryUsage"] = memoryUsage;
        doc["diskUsage"] = diskUsage;
        doc["deltaTime"] = deltaTime;
        doc["uptime"] = uptime;

        doc["light"] = light;
        doc["lightEnum"] = lightEnum;

        String output;
        serializeJson(doc, output);
        return output;
    }
};

#endif