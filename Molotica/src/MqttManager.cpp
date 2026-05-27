#include "MqttManager.h"
#include <ArduinoJson.h>

MqttManager* MqttManager::s_instance = nullptr;

MqttManager::MqttManager(StateMachine* sm, ErrorHandler* err, MessageBuffer* msg_buf)
    : m_sm(sm), m_err(err), m_msg_buf(msg_buf),
      m_mqtt_client(m_wifi_client), m_connected(false),
      m_last_reconnect(0), m_reconnect_delay(MQTT_RECONNECT_MS) {
    s_instance = this;
    m_mac[0] = '\0';
}

void MqttManager::getDeviceMac(char* mac_out, size_t max_len) {
    uint64_t mac = ESP.getEfuseMac();
    snprintf(mac_out, max_len, "%02X:%02X:%02X:%02X:%02X:%02X",
        (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
        (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
}

bool MqttManager::init() {
    m_mqtt_client.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    m_mqtt_client.setCallback(mqttCallback);
    m_mqtt_client.setBufferSize(512);
    getDeviceMac(m_mac, sizeof(m_mac));
    return true;
}

bool MqttManager::connect() {
    char client_id[32];
    snprintf(client_id, sizeof(client_id), "nodealert-%06x", (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));

    char will_topic[64];
    snprintf(will_topic, sizeof(will_topic), "%s/%s/status", MQTT_TOPIC_PREFIX, DEVICE_ID);

    if (m_mqtt_client.connect(client_id, MQTT_USER, MQTT_PASS,
                              will_topic, 1, true, "offline")) {
        handleConnected();
        m_reconnect_delay = MQTT_RECONNECT_MS;
        return true;
    }
    return false;
}

bool MqttManager::disconnect() {
    publishStatus("offline");
    m_mqtt_client.disconnect();
    m_connected = false;
    if (m_sm) m_sm->setState(SystemState::MQTT_CONNECTING);
    return true;
}

bool MqttManager::publish(const char* topic, const char* data, bool retain) {
    if (!m_connected) return false;
    return m_mqtt_client.publish(topic, data, retain);
}

bool MqttManager::subscribe(const char* topic) {
    if (!m_connected) return false;
    return m_mqtt_client.subscribe(topic);
}

bool MqttManager::isConnected() { return m_mqtt_client.connected(); }

bool MqttManager::publishStatus(const char* status) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/status", MQTT_TOPIC_PREFIX, DEVICE_ID);

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["status"] = status;
    doc["mac"] = m_mac;
    doc["ts"] = millis();

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));
    return publish(topic, buffer, true);
}

bool MqttManager::publishSensorData(const SensorReading& reading) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/telemetry", MQTT_TOPIC_PREFIX, DEVICE_ID);

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["mac"] = m_mac;
    doc["timestamp"] = millis();

    if (reading.dht_valid) {
        doc["temperature"] = reading.temperature;
        doc["humidity"] = reading.humidity;
    }
    if (reading.mq9_valid) {
        doc["gas_ppm"] = reading.gas_raw;
    }
    if (reading.ky026_valid) {
        doc["flame_detected"] = reading.flame_raw;
    }

    char buffer[512];
    serializeJson(doc, buffer, sizeof(buffer));
    return publish(topic, buffer, false);
}

bool MqttManager::publishAlert(const AlertMessage& alert) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/events", MQTT_TOPIC_PREFIX, DEVICE_ID);

    const char* event_type = "unknown";
    const char* severity = "warning";
    switch (alert.type) {
        case AlertType::FIRE:
            event_type = "fire_detected";
            severity = "critical";
            break;
        case AlertType::GAS_LEAK:
            event_type = "gas_alert";
            severity = "critical";
            break;
        case AlertType::HIGH_TEMP:
            event_type = "threshold_crossed";
            severity = "warning";
            break;
        case AlertType::LOW_TEMP:
            event_type = "threshold_crossed";
            severity = "warning";
            break;
        case AlertType::HIGH_HUMIDITY:
            event_type = "threshold_crossed";
            severity = "warning";
            break;
        case AlertType::LOW_HUMIDITY:
            event_type = "threshold_crossed";
            severity = "warning";
            break;
        default:
            break;
    }

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["mac"] = m_mac;
    doc["event_type"] = event_type;
    doc["severity"] = severity;
    doc["message"] = alert.description;
    doc["timestamp"] = alert.timestamp;

    char buffer[512];
    serializeJson(doc, buffer, sizeof(buffer));
    return publish(topic, buffer, false);
}

bool MqttManager::publishLog(const char* message) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/log", MQTT_TOPIC_PREFIX, DEVICE_ID);
    return publish(topic, message);
}

void MqttManager::processIncomingCommand(const char* data, int data_len) {
    if (data_len <= 0 || !data) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, data_len);
    if (err) return;

    const char* cmd = doc["cmd"] | "";
    if (strlen(cmd) == 0) {
        cmd = doc["command"] | "";
    }
    if (strlen(cmd) == 0) return;

    if (strcmp(cmd, "reboot") == 0) {
        publishLog("Rebooting by command");
        delay(100);
        ESP.restart();
    } else if (strcmp(cmd, "calibrate") == 0) {
        publishLog("Calibration triggered by command");
    } else if (strcmp(cmd, "status") == 0) {
        publishStatus("online");
    } else if (strcmp(cmd, "buzzer_on") == 0) {
        publishLog("Buzzer ON by command");
    } else if (strcmp(cmd, "buzzer_off") == 0) {
        publishLog("Buzzer OFF by command");
    } else if (strcmp(cmd, "return_to_auto") == 0) {
        publishLog("Return to auto by command");
    } else if (strcmp(cmd, "acknowledge_alarm") == 0) {
        publishLog("Alarm acknowledged by command");
    }
}

void MqttManager::update() {
    if (!m_mqtt_client.connected()) {
        handleDisconnected();
        unsigned long now = millis();
        if (now - m_last_reconnect >= m_reconnect_delay) {
            m_last_reconnect = now;
            if (WiFi.status() == WL_CONNECTED) {
                if (connect()) {
                    m_reconnect_delay = MQTT_RECONNECT_MS;
                } else {
                    m_reconnect_delay *= 2;
                    if (m_reconnect_delay > MQTT_RECONNECT_MAX) {
                        m_reconnect_delay = MQTT_RECONNECT_MAX;
                    }
                }
            }
        }
    } else {
        m_mqtt_client.loop();
        if (!m_connected) handleConnected();
    }
}

void MqttManager::handleConnected() {
    m_connected = true;
    if (m_sm) m_sm->setState(SystemState::MQTT_CONNECTED);
    subscribeToCommandTopic();
    publishStatus("online");
}

void MqttManager::handleDisconnected() {
    if (m_connected) {
        m_connected = false;
        if (m_sm) m_sm->setState(SystemState::MQTT_CONNECTING);
    }
}

void MqttManager::handleData(const char* topic, const uint8_t* data, unsigned int len) {
    processIncomingCommand((const char*)data, (int)len);
}

void MqttManager::subscribeToCommandTopic() {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/commands", MQTT_TOPIC_PREFIX, DEVICE_ID);
    subscribe(topic);
}

void MqttManager::mqttCallback(char* topic, uint8_t* payload, unsigned int len) {
    if (s_instance) {
        s_instance->handleData(topic, payload, len);
    }
}
