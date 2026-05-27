#include "Sensors.h"
#include <Preferences.h>

static Preferences sensorPrefs;

DHT22Driver::DHT22Driver(uint8_t pin)
    : m_pin(pin), m_dht(pin, DHT22), m_last_temp(NAN), m_last_hum(NAN),
      m_last_error(0), m_last_read(0) {}

bool DHT22Driver::init() {
    m_dht.begin();
    delay(1000);
    float t = m_dht.readTemperature();
    if (isnan(t)) {
        m_last_error = 1;
        return false;
    }
    m_last_temp = t;
    m_last_hum = m_dht.readHumidity();
    m_last_read = millis();
    return true;
}

bool DHT22Driver::read(float* temperature, float* humidity) {
    unsigned long now = millis();
    if (now - m_last_read < READ_INTERVAL) {
        *temperature = m_last_temp;
        *humidity = m_last_hum;
        return true;
    }
    m_last_temp = m_dht.readTemperature();
    m_last_hum = m_dht.readHumidity();
    m_last_read = now;
    if (isnan(m_last_temp) || isnan(m_last_hum)) {
        m_last_error = 1;
        return false;
    }
    *temperature = m_last_temp;
    *humidity = m_last_hum;
    return true;
}

float DHT22Driver::getTemperature() { return m_last_temp; }
float DHT22Driver::getHumidity() { return m_last_hum; }
int DHT22Driver::getLastError() { return m_last_error; }

MQ9Driver::MQ9Driver(uint8_t analog_pin, uint8_t digital_pin, uint8_t heater_pin)
    : m_analog_pin(analog_pin), m_digital_pin(digital_pin), m_heater_pin(heater_pin),
      m_baseline(0), m_last_error(0), m_heater_start(0), m_heater_on(false) {}

bool MQ9Driver::init() {
    pinMode(m_analog_pin, INPUT);
    pinMode(m_digital_pin, INPUT);
    pinMode(m_heater_pin, OUTPUT);
    digitalWrite(m_heater_pin, HIGH);
    m_heater_on = true;
    m_heater_start = millis();
    return true;
}

int MQ9Driver::readRaw() {
    return analogRead(m_analog_pin);
}

float MQ9Driver::readScaled() {
    return analogRead(m_analog_pin) * 3.3f / 4095.0f;
}

bool MQ9Driver::isGasDetected() {
    return digitalRead(m_digital_pin) == LOW;
}

void MQ9Driver::calibrate() {
    const int samples = 50;
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(m_analog_pin);
        delay(10);
    }
    m_baseline = sum / samples;
}

int MQ9Driver::getBaseline() const { return m_baseline; }
void MQ9Driver::setBaseline(int baseline) { m_baseline = baseline; }
int MQ9Driver::getLastError() { return m_last_error; }

void MQ9Driver::setHeater(bool state) {
    digitalWrite(m_heater_pin, state ? HIGH : LOW);
    m_heater_on = state;
}

KY026Driver::KY026Driver(uint8_t analog_pin, uint8_t digital_pin)
    : m_analog_pin(analog_pin), m_digital_pin(digital_pin),
      m_baseline(0), m_last_error(0) {}

bool KY026Driver::init() {
    pinMode(m_analog_pin, INPUT);
    pinMode(m_digital_pin, INPUT);
    return true;
}

int KY026Driver::readRaw() {
    return analogRead(m_analog_pin);
}

bool KY026Driver::isFlameDetected() {
    return digitalRead(m_digital_pin) == LOW;
}

int KY026Driver::getBaseline() const { return m_baseline; }
void KY026Driver::setBaseline(int baseline) { m_baseline = baseline; }
int KY026Driver::getLastError() { return m_last_error; }

bool SensorCalibration::loadCalibration() {
    sensorPrefs.begin("nodealert", true);
    m_mq9_baseline = sensorPrefs.getInt("mq9_bl", 0);
    m_ky026_baseline = sensorPrefs.getInt("ky026_bl", 0);
    m_temp_offset = sensorPrefs.getFloat("temp_off", 0.0f);
    m_hum_offset = sensorPrefs.getFloat("hum_off", 0.0f);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::saveCalibration() {
    sensorPrefs.begin("nodealert", false);
    sensorPrefs.putInt("mq9_bl", m_mq9_baseline);
    sensorPrefs.putInt("ky026_bl", m_ky026_baseline);
    sensorPrefs.putFloat("temp_off", m_temp_offset);
    sensorPrefs.putFloat("hum_off", m_hum_offset);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::loadDHTTemperatureOffset() {
    sensorPrefs.begin("nodealert", true);
    m_temp_offset = sensorPrefs.getFloat("temp_off", 0.0f);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::loadDHTHumidityOffset() {
    sensorPrefs.begin("nodealert", true);
    m_hum_offset = sensorPrefs.getFloat("hum_off", 0.0f);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::loadMQ9Calibration() {
    sensorPrefs.begin("nodealert", true);
    m_mq9_baseline = sensorPrefs.getInt("mq9_bl", 0);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::loadKY026Calibration() {
    sensorPrefs.begin("nodealert", true);
    m_ky026_baseline = sensorPrefs.getInt("ky026_bl", 0);
    sensorPrefs.end();
    return true;
}

bool SensorCalibration::saveDHTTemperatureOffset(float offset) {
    sensorPrefs.begin("nodealert", false);
    sensorPrefs.putFloat("temp_off", offset);
    sensorPrefs.end();
    m_temp_offset = offset;
    return true;
}

bool SensorCalibration::saveDHTHumidityOffset(float offset) {
    sensorPrefs.begin("nodealert", false);
    sensorPrefs.putFloat("hum_off", offset);
    sensorPrefs.end();
    m_hum_offset = offset;
    return true;
}

bool SensorCalibration::saveMQ9Calibration(int baseline) {
    sensorPrefs.begin("nodealert", false);
    sensorPrefs.putInt("mq9_bl", baseline);
    sensorPrefs.end();
    m_mq9_baseline = baseline;
    return true;
}

bool SensorCalibration::saveKY026Calibration(int baseline) {
    sensorPrefs.begin("nodealert", false);
    sensorPrefs.putInt("ky026_bl", baseline);
    sensorPrefs.end();
    m_ky026_baseline = baseline;
    return true;
}

float SensorCalibration::getTemperatureOffset() const { return m_temp_offset; }
float SensorCalibration::getHumidityOffset() const { return m_hum_offset; }
int SensorCalibration::getMQ9Baseline() const { return m_mq9_baseline; }
int SensorCalibration::getKY026Baseline() const { return m_ky026_baseline; }
void SensorCalibration::setTemperatureOffset(float offset) { m_temp_offset = offset; }
void SensorCalibration::setHumidityOffset(float offset) { m_hum_offset = offset; }
void SensorCalibration::setMQ9Baseline(int baseline) { m_mq9_baseline = baseline; }
void SensorCalibration::setKY026Baseline(int baseline) { m_ky026_baseline = baseline; }

void SensorCalibration::reset() {
    m_temp_offset = 0.0f;
    m_hum_offset = 0.0f;
    m_mq9_baseline = 0;
    m_ky026_baseline = 0;
    saveCalibration();
}
