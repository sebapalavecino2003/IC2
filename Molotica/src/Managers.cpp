#include "Managers.h"

SensorManager::SensorManager(DHT22Driver* dht, MQ9Driver* mq9, KY026Driver* ky026,
                             SensorCalibration* cal, MessageBuffer* buf,
                             ErrorHandler* err, SerialManager* log)
    : m_dht(dht), m_mq9(mq9), m_ky026(ky026), m_cal(cal), m_buf(buf),
      m_err(err), m_log(log), m_dht_ready(false), m_mq9_ready(false), m_ky026_ready(false) {}

bool SensorManager::init() {
    if (m_dht) m_dht_ready = m_dht->init();
    if (m_mq9) m_mq9_ready = m_mq9->init();
    if (m_ky026) m_ky026_ready = m_ky026->init();
    return m_dht_ready || m_mq9_ready || m_ky026_ready;
}

SensorReading SensorManager::readAll() {
    SensorReading reading;
    reading.timestamp = millis();
    if (m_dht_ready && m_dht) {
        m_dht->read(&reading.temperature, &reading.humidity);
        reading.dht_valid = !isnan(reading.temperature) && !isnan(reading.humidity);
    } else {
        reading.dht_valid = false;
    }
    if (m_mq9_ready && m_mq9) {
        reading.gas_raw = m_mq9->readRaw();
        reading.mq9_valid = true;
    } else {
        reading.mq9_valid = false;
    }
    if (m_ky026_ready && m_ky026) {
        reading.flame_raw = m_ky026->readRaw();
        reading.ky026_valid = true;
    } else {
        reading.ky026_valid = false;
    }
    validateReadings(&reading);
    m_last_reading = reading;
    return reading;
}

void SensorManager::calibrateAll() {
    if (m_mq9 && m_cal) {
        m_mq9->calibrate();
        m_cal->saveMQ9Calibration(m_mq9->getBaseline());
    }
    if (m_ky026 && m_cal) {
        m_cal->saveKY026Calibration(m_ky026->getBaseline());
    }
}

void SensorManager::updateCalibration() {
    if (!m_cal) return;
    if (m_mq9) {
        int baseline = m_cal->loadMQ9Calibration();
        if (baseline > 0) m_mq9->setBaseline(baseline);
    }
    if (m_ky026) {
        int baseline = m_cal->loadKY026Calibration();
        if (baseline > 0) m_ky026->setBaseline(baseline);
    }
}

bool SensorManager::isDHTReady() const { return m_dht_ready; }
bool SensorManager::isMQ9Ready() const { return m_mq9_ready; }
bool SensorManager::isKY026Ready() const { return m_ky026_ready; }
float SensorManager::getLastTemp() const { return m_last_reading.temperature; }
float SensorManager::getLastHum() const { return m_last_reading.humidity; }
int SensorManager::getLastGas() const { return m_last_reading.gas_raw; }
int SensorManager::getLastFlame() const { return m_last_reading.flame_raw; }
const SensorReading& SensorManager::getLastReading() const { return m_last_reading; }

void SensorManager::update() {}

void SensorManager::validateReadings(SensorReading* reading) {
    if (!reading->dht_valid) {
        reading->temperature = -1;
        reading->humidity = -1;
    }
    if (!reading->mq9_valid) reading->gas_raw = -1;
    if (!reading->ky026_valid) reading->flame_raw = -1;
}

void SensorManager::checkAlerts(const SensorReading& reading) {
    if (!reading.dht_valid) {
        m_err->logError(ErrorCode::DHT_FAIL, "DHT22 read failed");
    }
    if (!reading.mq9_valid) {
        m_err->logError(ErrorCode::MQ9_FAIL, "MQ-9 read failed");
    }
    if (!reading.ky026_valid) {
        m_err->logError(ErrorCode::KY026_FAIL, "KY-026 read failed");
    }
}

bool SensorManager::isTempInRange(float temp) const {
    return temp >= g_thresholds.temp_min && temp <= g_thresholds.temp_max;
}

bool SensorManager::isHumInRange(float hum) const {
    return hum >= g_thresholds.humidity_min && hum <= g_thresholds.humidity_max;
}

bool SensorManager::isGasAboveThreshold(int gas) const {
    return gas > g_thresholds.gas_threshold;
}

bool SensorManager::isFlameAboveThreshold(int flame) const {
    return flame > g_thresholds.flame_threshold;
}

AutomationManager::AutomationManager(StateMachine* sm, MessageBuffer* buf, ErrorHandler* err,
                                     MqttManager* mqtt, BuzzerManager* buzzer, SerialManager* log)
    : m_sm(sm), m_buf(buf), m_err(err), m_mqtt(mqtt), m_buzzer(buzzer), m_log(log),
      m_current_alert(AlertType::NONE), m_last_eval(0) {}

void AutomationManager::evaluate(const SensorReading& reading) {
    m_last_eval = millis();
    if (reading.ky026_valid && reading.flame_raw > g_thresholds.flame_threshold) {
        handleAlert(AlertType::FIRE, "Flame detected");
        triggerFireAlert();
        return;
    }
    if (reading.mq9_valid && reading.gas_raw > g_thresholds.gas_threshold) {
        handleAlert(AlertType::GAS_LEAK, "Gas leak detected");
        triggerGasAlert();
        return;
    }
    if (reading.dht_valid) {
        if (reading.temperature > g_thresholds.temp_max) {
            handleAlert(AlertType::HIGH_TEMP, "High temperature");
            triggerTempAlert(reading.temperature);
            return;
        }
        if (reading.temperature < g_thresholds.temp_min) {
            handleAlert(AlertType::LOW_TEMP, "Low temperature");
            triggerTempAlert(reading.temperature);
            return;
        }
        if (reading.humidity > g_thresholds.humidity_max) {
            handleAlert(AlertType::HIGH_HUMIDITY, "High humidity");
            triggerHumAlert(reading.humidity);
            return;
        }
        if (reading.humidity < g_thresholds.humidity_min) {
            handleAlert(AlertType::LOW_HUMIDITY, "Low humidity");
            triggerHumAlert(reading.humidity);
            return;
        }
    }
    clearAlert();
}

void AutomationManager::handleAlert(AlertType type, const char* desc) {
    m_current_alert = type;
    AlertMessage msg;
    msg.timestamp = millis();
    msg.type = type;
    strncpy(msg.description, desc, sizeof(msg.description) - 1);
    msg.description[sizeof(msg.description) - 1] = '\0';
    msg.value = 0;
    m_buf->pushAlert(msg);
    if (m_mqtt) m_mqtt->publishAlert(msg);
    m_sm->setState(SystemState::ALERT);
    if (m_log) m_log->printAlert(msg);
}

void AutomationManager::clearAlert() {
    if (m_current_alert == AlertType::NONE) return;
    m_current_alert = AlertType::NONE;
    m_sm->setState(SystemState::MONITORING);
}

AlertType AutomationManager::getCurrentAlert() const { return m_current_alert; }

void AutomationManager::reset() {
    m_current_alert = AlertType::NONE;
    m_last_eval = 0;
}

void AutomationManager::update() {}

void AutomationManager::triggerFireAlert() {
    m_buzzer->playAlertTone(AlertType::FIRE);
}

void AutomationManager::triggerGasAlert() {
    m_buzzer->playAlertTone(AlertType::GAS_LEAK);
}

void AutomationManager::triggerTempAlert(float temp) {
    m_buzzer->playAlertTone(AlertType::HIGH_TEMP);
}

void AutomationManager::triggerHumAlert(float hum) {
    m_buzzer->playAlertTone(AlertType::HIGH_HUMIDITY);
}
