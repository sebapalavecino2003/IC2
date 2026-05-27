#pragma once

#include <Arduino.h>
#include <DHT.h>

#include "Config.h"

class DHT22Driver {
public:
    DHT22Driver(uint8_t pin);
    bool init();
    bool read(float* temperature, float* humidity);
    float getTemperature();
    float getHumidity();
    int getLastError();
private:
    uint8_t m_pin;
    DHT m_dht;
    float m_last_temp;
    float m_last_hum;
    int m_last_error;
    unsigned long m_last_read;
    static const unsigned long READ_INTERVAL = 2000;
};

class MQ9Driver {
public:
    MQ9Driver(uint8_t analog_pin, uint8_t digital_pin, uint8_t heater_pin);
    bool init();
    int readRaw();
    float readScaled();
    bool isGasDetected();
    void calibrate();
    int getBaseline() const;
    void setBaseline(int baseline);
    int getLastError();
    void setHeater(bool state);
private:
    uint8_t m_analog_pin;
    uint8_t m_digital_pin;
    uint8_t m_heater_pin;
    int m_baseline;
    int m_last_error;
    unsigned long m_heater_start;
    bool m_heater_on;
};

class KY026Driver {
public:
    KY026Driver(uint8_t analog_pin, uint8_t digital_pin);
    bool init();
    int readRaw();
    bool isFlameDetected();
    int getBaseline() const;
    void setBaseline(int baseline);
    int getLastError();
private:
    uint8_t m_analog_pin;
    uint8_t m_digital_pin;
    int m_baseline;
    int m_last_error;
};

class SensorCalibration {
public:
    bool loadCalibration();
    bool saveCalibration();
    bool loadDHTTemperatureOffset();
    bool loadDHTHumidityOffset();
    bool loadMQ9Calibration();
    bool loadKY026Calibration();
    bool saveDHTTemperatureOffset(float offset);
    bool saveDHTHumidityOffset(float offset);
    bool saveMQ9Calibration(int baseline);
    bool saveKY026Calibration(int baseline);
    float getTemperatureOffset() const;
    float getHumidityOffset() const;
    int getMQ9Baseline() const;
    int getKY026Baseline() const;
    void setTemperatureOffset(float offset);
    void setHumidityOffset(float offset);
    void setMQ9Baseline(int baseline);
    void setKY026Baseline(int baseline);
    void reset();
private:
    float m_temp_offset;
    float m_hum_offset;
    int m_mq9_baseline;
    int m_ky026_baseline;
};
