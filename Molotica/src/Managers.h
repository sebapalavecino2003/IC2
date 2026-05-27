#pragma once

#include "Core.h"
#include "Sensors.h"
#include "MqttManager.h"

class SensorManager {
public:
    SensorManager(DHT22Driver* dht, MQ9Driver* mq9, KY026Driver* ky026, SensorCalibration* cal,
                  MessageBuffer* buf, ErrorHandler* err, SerialManager* log);
    bool init();
    SensorReading readAll();
    void calibrateAll();
    void updateCalibration();
    bool isDHTReady() const;
    bool isMQ9Ready() const;
    bool isKY026Ready() const;
    float getLastTemp() const;
    float getLastHum() const;
    int getLastGas() const;
    int getLastFlame() const;
    const SensorReading& getLastReading() const;
    void update();
private:
    DHT22Driver* m_dht;
    MQ9Driver* m_mq9;
    KY026Driver* m_ky026;
    SensorCalibration* m_cal;
    MessageBuffer* m_buf;
    ErrorHandler* m_err;
    SerialManager* m_log;
    SensorReading m_last_reading;
    bool m_dht_ready, m_mq9_ready, m_ky026_ready;
    void validateReadings(SensorReading* reading);
    void checkAlerts(const SensorReading& reading);
    bool isTempInRange(float temp) const;
    bool isHumInRange(float hum) const;
    bool isGasAboveThreshold(int gas) const;
    bool isFlameAboveThreshold(int flame) const;
};

class AutomationManager {
public:
    AutomationManager(StateMachine* sm, MessageBuffer* buf, ErrorHandler* err,
                      MqttManager* mqtt, BuzzerManager* buzzer, SerialManager* log);
    void evaluate(const SensorReading& reading);
    void handleAlert(AlertType type, const char* desc);
    void clearAlert();
    AlertType getCurrentAlert() const;
    void reset();
    void update();
private:
    StateMachine* m_sm;
    MessageBuffer* m_buf;
    ErrorHandler* m_err;
    MqttManager* m_mqtt;
    BuzzerManager* m_buzzer;
    SerialManager* m_log;
    AlertType m_current_alert;
    unsigned long m_last_eval;
    void triggerFireAlert();
    void triggerGasAlert();
    void triggerTempAlert(float temp);
    void triggerHumAlert(float hum);
};
