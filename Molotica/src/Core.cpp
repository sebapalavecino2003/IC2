#include "Core.h"
#include <Preferences.h>
#include <esp32-hal-ledc.h>

static Preferences preferences;

StateMachine::StateMachine() : m_current_state(SystemState::INIT) {}

void StateMachine::setState(SystemState new_state) {
    m_current_state = new_state;
}

SystemState StateMachine::getState() const {
    return m_current_state;
}

bool StateMachine::isInState(SystemState state) const {
    return m_current_state == state;
}

bool StateMachine::isAtLeast(SystemState state) const {
    return m_current_state >= state;
}

const char* StateMachine::stateToString() const {
    switch (m_current_state) {
        case SystemState::INIT: return "INIT";
        case SystemState::WIFI_CONNECTING: return "WIFI_CONNECTING";
        case SystemState::WIFI_CONNECTED: return "WIFI_CONNECTED";
        case SystemState::MQTT_CONNECTING: return "MQTT_CONNECTING";
        case SystemState::MQTT_CONNECTED: return "MQTT_CONNECTED";
        case SystemState::SENSOR_READY: return "SENSOR_READY";
        case SystemState::MONITORING: return "MONITORING";
        case SystemState::ALERT: return "ALERT";
        case SystemState::ERROR: return "ERROR";
        case SystemState::SHUTDOWN: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

ErrorHandler::ErrorHandler() : m_error_count(0) {
    m_last_error.code = ErrorCode::NONE;
    m_last_error.timestamp = 0;
    m_last_error.message[0] = '\0';
}

void ErrorHandler::logError(ErrorCode code, const char* msg) {
    if (m_error_count < 10) {
        m_errors[m_error_count].timestamp = millis();
        m_errors[m_error_count].code = code;
        strncpy(m_errors[m_error_count].message, msg, sizeof(m_errors[m_error_count].message) - 1);
        m_errors[m_error_count].message[sizeof(m_errors[m_error_count].message) - 1] = '\0';
        m_error_count++;
    }
    m_last_error.timestamp = millis();
    m_last_error.code = code;
    strncpy(m_last_error.message, msg, sizeof(m_last_error.message) - 1);
    m_last_error.message[sizeof(m_last_error.message) - 1] = '\0';
}

void ErrorHandler::clearErrors() { m_error_count = 0; }
bool ErrorHandler::hasError() const { return m_error_count > 0; }
int ErrorHandler::getErrorCount() const { return m_error_count; }
ErrorEvent ErrorHandler::getLastError() const { return m_last_error; }

void ErrorHandler::reset() {
    m_error_count = 0;
    m_last_error.code = ErrorCode::NONE;
    m_last_error.timestamp = 0;
    m_last_error.message[0] = '\0';
}

MessageBuffer::MessageBuffer()
    : m_log_head(0), m_log_tail(0), m_log_count(0),
      m_alert_head(0), m_alert_tail(0), m_alert_count(0),
      m_read_head(0), m_read_tail(0), m_read_count(0),
      m_last_alert_time(0) {}

bool MessageBuffer::pushLog(const LogEntry& entry) {
    if (m_log_count >= 20) return false;
    m_logs[m_log_head] = entry;
    m_log_head = (m_log_head + 1) % 20;
    m_log_count++;
    return true;
}

bool MessageBuffer::pushAlert(const AlertMessage& alert) {
    if (m_alert_count >= 10) return false;
    m_alerts[m_alert_head] = alert;
    m_alert_head = (m_alert_head + 1) % 10;
    m_alert_count++;
    m_last_alert_time = alert.timestamp;
    return true;
}

bool MessageBuffer::pushSensorReading(const SensorReading& reading) {
    if (m_read_count >= 5) return false;
    m_readings[m_read_head] = reading;
    m_read_head = (m_read_head + 1) % 5;
    m_read_count++;
    return true;
}

bool MessageBuffer::popLog(LogEntry* entry) {
    if (m_log_count == 0) return false;
    *entry = m_logs[m_log_tail];
    m_log_tail = (m_log_tail + 1) % 20;
    m_log_count--;
    return true;
}

bool MessageBuffer::popAlert(AlertMessage* alert) {
    if (m_alert_count == 0) return false;
    *alert = m_alerts[m_alert_tail];
    m_alert_tail = (m_alert_tail + 1) % 10;
    m_alert_count--;
    return true;
}

bool MessageBuffer::popSensorReading(SensorReading* reading) {
    if (m_read_count == 0) return false;
    *reading = m_readings[m_read_tail];
    m_read_tail = (m_read_tail + 1) % 5;
    m_read_count--;
    return true;
}

bool MessageBuffer::hasLog() const { return m_log_count > 0; }
bool MessageBuffer::hasAlert() const { return m_alert_count > 0; }
bool MessageBuffer::hasSensorReading() const { return m_read_count > 0; }
unsigned long MessageBuffer::getLastAlertTime() const { return m_last_alert_time; }

BuzzerManager::BuzzerManager() : m_active(false), m_frequency(0), m_end_time(0) {}

void BuzzerManager::init() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void BuzzerManager::playTone(int frequency, unsigned long duration_ms) {
    m_frequency = frequency;
    m_end_time = millis() + duration_ms;
    m_active = true;
}

void BuzzerManager::playAlertTone(AlertType type) {
    switch (type) {
        case AlertType::FIRE:
            beep(4000, 100);
            break;
        case AlertType::GAS_LEAK:
            beep(2000, 200);
            break;
        case AlertType::HIGH_TEMP:
            beep(1000, 500);
            break;
        default:
            beep(500, 300);
            break;
    }
}

void BuzzerManager::stop() {
    m_active = false;
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
    ledcDetachPin(BUZZER_PIN);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void BuzzerManager::update() {
    if (!m_active) return;
    if (millis() >= m_end_time || m_end_time == 0) {
        stop();
    }
}

bool BuzzerManager::isPlaying() const { return m_active; }

void BuzzerManager::beep(int freq, unsigned long duration_ms) {
    if (freq <= 0 || duration_ms == 0) return;
    ledcSetup(BUZZER_LEDC_CHANNEL, freq, BUZZER_LEDC_TIMER);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 128);
    m_end_time = millis() + duration_ms;
    m_active = true;
}

WifiManager::WifiManager() : m_connected(false), m_retry_start(0), m_retry_count(0) {}

bool WifiManager::init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    m_retry_start = millis();
    m_retry_count = 0;
    return true;
}

void WifiManager::update() {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        if (!m_connected) {
            m_connected = true;
            m_retry_count = 0;
        }
        return;
    }
    if (m_connected) {
        m_connected = false;
    }
    if (status == WL_DISCONNECTED || status == WL_CONNECT_FAILED ||
        status == WL_CONNECTION_LOST || status == WL_IDLE_STATUS) {
        if (m_retry_count < WIFI_MAX_RETRIES) {
            unsigned long now = millis();
            if (now - m_retry_start >= WIFI_RETRY_MS) {
                m_retry_start = now;
                WiFi.reconnect();
                m_retry_count++;
            }
        }
    }
}

bool WifiManager::isConnected() const { return m_connected; }

void WifiManager::disconnect() {
    WiFi.disconnect();
    m_connected = false;
}

const char* WifiManager::getSSID() const { return WIFI_SSID; }
IPAddress WifiManager::getIP() const { return WiFi.localIP(); }

SerialManager::SerialManager() {}

void SerialManager::init(unsigned long baud) {
    Serial.begin(baud);
}

void SerialManager::printInfo(const char* tag, const char* msg) {
    Serial.print("[INFO][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
}

void SerialManager::printWarning(const char* tag, const char* msg) {
    Serial.print("[WARN][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
}

void SerialManager::printError(const char* tag, const char* msg) {
    Serial.print("[ERR][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
}

void SerialManager::printSensor(const SensorReading& reading) {
    Serial.print("[SENSOR] t=");
    Serial.print(reading.temperature);
    Serial.print(" h=");
    Serial.print(reading.humidity);
    Serial.print(" gas=");
    Serial.print(reading.gas_raw);
    Serial.print(" flame=");
    Serial.println(reading.flame_raw);
}

void SerialManager::printAlert(const AlertMessage& alert) {
    Serial.print("[ALERT] type=");
    Serial.print((int)alert.type);
    Serial.print(" desc=");
    Serial.print(alert.description);
    Serial.print(" val=");
    Serial.println(alert.value);
}

void SerialManager::printLog(const char* message) {
    Serial.println(message);
}
