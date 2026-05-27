#pragma once

#include "Core.h"
#include <ArduinoJson.h>

int extractCommand(const char* json_str, char* cmd_out, int cmd_size);
bool findJsonKey(const char* json_str, const char* key, char* value_out, int value_size);
bool buildStatusJson(char* buffer, int buffer_size, const char* state, const SensorReading* reading, bool wifi_ok, bool mqtt_ok);
bool buildAlertJson(char* buffer, int buffer_size, const AlertMessage& alert);
bool buildLogJson(char* buffer, int buffer_size, const char* message);
