#pragma once

#include <stdint.h>
#include <stddef.h>
#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"

enum class SystemState : uint8_t {
    INIT,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    SENSOR_READY,
    MONITORING,
    ALERT,
    ERROR,
    SHUTDOWN
};

enum class AlertType : uint8_t {
    NONE,
    FIRE,
    GAS_LEAK,
    HIGH_TEMP,
    LOW_TEMP,
    HIGH_HUMIDITY,
    LOW_HUMIDITY,
    SENSOR_FAILURE,
    SYSTEM_ERROR
};

enum class ErrorCode : uint8_t {
    NONE,
    WIFI_FAIL,
    MQTT_FAIL,
    DHT_FAIL,
    MQ9_FAIL,
    KY026_FAIL,
    SENSOR_TIMEOUT,
    INVALID_STATE,
    CALIBRATION_FAIL,
    UNKNOWN
};

typedef struct {
    unsigned long timestamp;
    ErrorCode code;
    char message[64];
} ErrorEvent;

typedef struct {
    float temperature;
    float humidity;
    int gas_raw;
    int flame_raw;
    unsigned long timestamp;
    bool dht_valid;
    bool mq9_valid;
    bool ky026_valid;
} SensorReading;

typedef struct {
    unsigned long timestamp;
    AlertType type;
    char description[64];
    float value;
} AlertMessage;

typedef struct {
    unsigned long timestamp;
    char message[64];
} LogEntry;

class StateMachine {
public:
    StateMachine();
    void setState(SystemState new_state);
    SystemState getState() const;
    bool isInState(SystemState state) const;
    bool isAtLeast(SystemState state) const;
    const char* stateToString() const;
private:
    SystemState m_current_state;
};

class ErrorHandler {
public:
    ErrorHandler();
    void logError(ErrorCode code, const char* msg);
    void clearErrors();
    bool hasError() const;
    int getErrorCount() const;
    ErrorEvent getLastError() const;
    void reset();
private:
    ErrorEvent m_errors[10];
    int m_error_count;
    ErrorEvent m_last_error;
};

class MessageBuffer {
public:
    MessageBuffer();
    bool pushLog(const LogEntry& entry);
    bool pushAlert(const AlertMessage& alert);
    bool pushSensorReading(const SensorReading& reading);
    bool popLog(LogEntry* entry);
    bool popAlert(AlertMessage* alert);
    bool popSensorReading(SensorReading* reading);
    bool hasLog() const;
    bool hasAlert() const;
    bool hasSensorReading() const;
    unsigned long getLastAlertTime() const;
private:
    LogEntry m_logs[20];
    AlertMessage m_alerts[10];
    SensorReading m_readings[5];
    int m_log_head, m_log_tail, m_log_count;
    int m_alert_head, m_alert_tail, m_alert_count;
    int m_read_head, m_read_tail, m_read_count;
    unsigned long m_last_alert_time;
};

class BuzzerManager {
public:
    BuzzerManager();
    void init();
    void playTone(int frequency, unsigned long duration_ms);
    void playAlertTone(AlertType type);
    void beep(int freq, unsigned long duration_ms);
    void stop();
    void update();
    bool isPlaying() const;
private:
    bool m_active;
    int m_frequency;
    unsigned long m_end_time;
};

class WifiManager {
public:
    WifiManager();
    bool init();
    void update();
    bool isConnected() const;
    void disconnect();
    const char* getSSID() const;
    IPAddress getIP() const;
private:
    bool m_connected;
    unsigned long m_retry_start;
    int m_retry_count;
};

class SerialManager {
public:
    SerialManager();
    void init(unsigned long baud = 115200);
    void printInfo(const char* tag, const char* msg);
    void printWarning(const char* tag, const char* msg);
    void printError(const char* tag, const char* msg);
    void printSensor(const SensorReading& reading);
    void printAlert(const AlertMessage& alert);
    void printLog(const char* message);
};
