#include "Utils.h"
#include <string.h>

int extractCommand(const char* json_str, char* cmd_out, int cmd_size) {
    if (!json_str || !cmd_out || cmd_size <= 0) return -1;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json_str);
    if (err) {
        strncpy(cmd_out, json_str, cmd_size - 1);
        cmd_out[cmd_size - 1] = '\0';
        return (int)strlen(cmd_out);
    }
    const char* cmd = doc["cmd"] | "";
    if (strlen(cmd) == 0) {
        cmd = doc["command"] | "";
    }
    strncpy(cmd_out, cmd, cmd_size - 1);
    cmd_out[cmd_size - 1] = '\0';
    return (int)strlen(cmd_out);
}

bool findJsonKey(const char* json_str, const char* key, char* value_out, int value_size) {
    if (!json_str || !key || !value_out || value_size <= 0) return false;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json_str);
    if (err) return false;
    const char* val = doc[key] | "";
    if (strlen(val) == 0) return false;
    strncpy(value_out, val, value_size - 1);
    value_out[value_size - 1] = '\0';
    return true;
}

bool buildStatusJson(char* buffer, int buffer_size, const char* state,
                     const SensorReading* reading, bool wifi_ok, bool mqtt_ok) {
    if (!buffer || buffer_size <= 0) return false;
    JsonDocument doc;
    doc["state"] = state;
    doc["ts"] = millis();
    if (reading) {
        doc["temperature"] = reading->temperature;
        doc["humidity"] = reading->humidity;
        doc["gas"] = reading->gas_raw;
        doc["flame"] = reading->flame_raw;
    }
    doc["wifi"] = wifi_ok;
    doc["mqtt"] = mqtt_ok;
    return serializeJson(doc, buffer, buffer_size) > 0;
}

bool buildAlertJson(char* buffer, int buffer_size, const AlertMessage& alert) {
    if (!buffer || buffer_size <= 0) return false;
    JsonDocument doc;
    doc["type"] = (int)alert.type;
    doc["desc"] = alert.description;
    doc["val"] = alert.value;
    doc["ts"] = alert.timestamp;
    return serializeJson(doc, buffer, buffer_size) > 0;
}

bool buildLogJson(char* buffer, int buffer_size, const char* message) {
    if (!buffer || buffer_size <= 0 || !message) return false;
    JsonDocument doc;
    doc["msg"] = message;
    doc["ts"] = millis();
    return serializeJson(doc, buffer, buffer_size) > 0;
}
