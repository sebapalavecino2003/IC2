#include "mqtt_manager.h"
#include "managers/automation_manager.h"
#include "core/error_handler.h"
#include "core/state_machine.h"
#include "core/message_buffer.h"
#include "config/mqtt_broker_config.h"
#include "hal/sensor_reading.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <cstdio>
#include <cstring>

MqttManager::MqttManager()
    : m_sensor_queue(nullptr)
    , m_error_handler(nullptr)
    , m_state_mach(nullptr)
    , m_task_handle(nullptr)
    , m_client(nullptr)
    , m_connected(false)
    , m_auto_mgr(nullptr)
    , m_last_status_ms(0)
    , m_last_state(SystemState::INIT)
{
}

void MqttManager::init(QueueHandle_t sensor_queue, ErrorHandler* eh, StateMachine* sm)
{
    m_sensor_queue  = sensor_queue;
    m_error_handler = eh;
    m_state_mach    = sm;
}

void MqttManager::startMqttTask()
{
    xTaskCreatePinnedToCore(mqttTask, "mqtt_task", 6144, this, 1, &m_task_handle, 0);
}

void MqttManager::buildTopic(const char* suffix, char* buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s%s/%s", MQTT_TOPIC_PREFIX, DEVICE_ID, suffix);
}

void MqttManager::buildTelemetryJson(const SensorReading& reading,
                                     char* buffer, size_t buffer_size)
{
    (void)reading;
}

void MqttManager::publishReading(const SensorReading& reading)
{
    char topic[64];
    char payload[MQTT_MESSAGE_MAX_LEN];

    buildTopic("telemetry", topic, sizeof(topic));

    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"timestamp\":%lu,\"temperature\":%.1f,"
        "\"humidity\":%.1f,\"gas_ppm\":%.0f,\"flame_detected\":%u}",
        DEVICE_ID,
        (unsigned long)(esp_timer_get_time() / 1000),
        (reading.type == SensorType::DHT22_TEMPERATURE) ? reading.value : 0.0f,
        (reading.type == SensorType::DHT22_HUMIDITY)    ? reading.value : 0.0f,
        (reading.type == SensorType::MQ9_GAS)           ? (double)reading.value : 0.0,
        (reading.type == SensorType::KY026_FLAME && reading.value > 0.5f) ? 1u : 0u);

    if (m_connected && m_client) {
        int ret = esp_mqtt_client_publish(m_client, topic, payload, 0, 0, 0);
        if (ret < 0) {
            m_buffer.enqueue(topic, payload, 0);
            m_error_handler->reportError("MQTT", "Publish failed — buffered");
        }
    } else {
        m_buffer.enqueue(topic, payload, 0);
    }
}

void MqttManager::drainBuffer()
{
    MqttMessage msg; // MessageBuffer::dequeue returns oldest buffered message
    int drained = 0;
    while (m_buffer.dequeue(msg)) {
        esp_mqtt_client_publish(m_client, msg.topic, msg.payload, 0, msg.qos, 0);
        drained++;
    }
    if (drained > 0) {
        printf("[MQTT] Drained %d buffered messages\n", drained);
    }
}

void MqttManager::buildStatusJson(char* buffer, size_t buffer_size)
{
    const char* state_str = "UNKNOWN";
    if (m_state_mach) {
        state_str = m_state_mach->getStateName();
    }

    uint32_t uptime_ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t free_heap = (uint32_t)esp_get_free_heap_size();

    int8_t rssi = 0;
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }

    int written = snprintf(buffer, buffer_size,
        "{\"device_id\":\"%s\",\"status\":\"online\",\"uptime_ms\":%lu,"
        "\"free_heap\":%lu,\"system_state\":\"%s\","
        "\"wifi_rssi\":%d,\"last_reset_reason\":%d,"
        "\"errores_activos\":[]}",
        DEVICE_ID, (unsigned long)uptime_ms,
        (unsigned long)free_heap, state_str,
        (int)rssi, (int)esp_reset_reason());

    (void)written;
}

void MqttManager::publishStatus()
{
    char topic[64];
    char payload[256];

    buildTopic("status", topic, sizeof(topic));
    buildStatusJson(payload, sizeof(payload));

    if (m_connected && m_client) {
        int ret = esp_mqtt_client_publish(m_client, topic, payload, 0, 1, 0);
        if (ret < 0) {
            m_error_handler->reportError("MQTT", "Status publish failed");
        }
    }
}

void MqttManager::mqttEventHandler(void* handler_args, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    (void)base;
    MqttManager* self = static_cast<MqttManager*>(handler_args);

    switch (event_id) {
        case MQTT_EVENT_CONNECTED: {
            self->m_connected = true;

            char cmd_topic[64];
            self->buildTopic("commands", cmd_topic, sizeof(cmd_topic));
            esp_mqtt_client_subscribe(self->m_client, cmd_topic, 1);

            if (self->m_auto_mgr) {
                self->m_auto_mgr->setMqttClient(self->m_client);
            }

            self->m_error_handler->clearError("MQTT");
            self->drainBuffer();
            break;
        }

        case MQTT_EVENT_DISCONNECTED: {
            self->m_connected = false;
            self->m_error_handler->reportError("MQTT", "Disconnected from broker");
            break;
        }

        case MQTT_EVENT_DATA: {
            esp_mqtt_event_t* msg = (esp_mqtt_event_t*)event_data;
            printf("[MQTT] Command: %.*s -> %.*s\n",
                   msg->topic_len, msg->topic,
                   msg->data_len, msg->data);

            if (self->m_auto_mgr && msg->data_len > 0) {
                char cmd[128];
                size_t len = (msg->data_len < 127) ? msg->data_len : 127;
                memcpy(cmd, msg->data, len);
                cmd[len] = '\0';
                self->m_auto_mgr->processCommand(cmd);
            }
            break;
        }

        default:
            break;
    }
}

void MqttManager::mqttTask(void* pvParams)
{
    MqttManager* self = static_cast<MqttManager*>(pvParams);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri       = MQTT_BROKER_URI;
    mqtt_cfg.broker.address.port      = MQTT_PORT;
    mqtt_cfg.credentials.username     = MQTT_USER;
    mqtt_cfg.credentials.authentication.password = MQTT_PASS;
    mqtt_cfg.session.keepalive        = MQTT_KEEPALIVE;
    mqtt_cfg.session.last_will.topic  = "nodealert/" DEVICE_ID "/status";
    mqtt_cfg.session.last_will.msg    = "{\"device_id\":\"" DEVICE_ID "\",\"status\":\"offline\",\"reason\":\"watchdog_reboot\"}";
    mqtt_cfg.session.last_will.qos    = 1;
    mqtt_cfg.session.last_will.retain = false;

    self->m_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(self->m_client, MQTT_EVENT_ANY,
                                   mqttEventHandler, self);
    esp_mqtt_client_start(self->m_client);

    SensorReading last_temp;
    SensorReading last_hum;
    SensorReading last_gas;
    SensorReading last_flame;

    uint32_t last_publish_ms = 0;

    while (1) {
        SensorReading reading;
        if (xQueueReceive(self->m_sensor_queue, &reading,
                          pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (reading.type) {
                case SensorType::DHT22_TEMPERATURE:
                    last_temp = reading;
                    break;
                case SensorType::DHT22_HUMIDITY:
                    last_hum = reading;
                    break;
                case SensorType::MQ9_GAS:
                    last_gas = reading;
                    break;
                case SensorType::KY026_FLAME:
                    last_flame = reading;
                    break;
            }
        }

        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if ((now_ms - last_publish_ms) >= 10000) {
            char topic[64];
            char payload[256];

            self->buildTopic("telemetry", topic, sizeof(topic));
            snprintf(payload, sizeof(payload),
                "{\"device_id\":\"%s\",\"timestamp\":%lu,\"temperature\":%.1f,"
                "\"humidity\":%.1f,\"gas_ppm\":%.0f,\"flame_detected\":%u}",
                DEVICE_ID, (unsigned long)now_ms,
                last_temp.value, last_hum.value, last_gas.value,
                (last_flame.value > 0.5f) ? 1u : 0u);

            if (self->m_connected && self->m_client) {
                int ret = esp_mqtt_client_publish(self->m_client, topic, payload, 0, 0, 0);
                if (ret < 0) {
                    self->m_buffer.enqueue(topic, payload, 0);
                    self->m_error_handler->reportError("MQTT", "Publish failed — buffered");
                }
            } else {
                self->m_buffer.enqueue(topic, payload, 0);
            }

            last_publish_ms = now_ms;
        }

        // Publish device status: every 60s OR immediately on state transition (D-13)
        SystemState current_state = self->m_state_mach->getCurrentState();
        bool state_changed = (current_state != self->m_last_state);
        bool time_elapsed  = ((now_ms - self->m_last_status_ms) >= 60000);

        if (state_changed || time_elapsed) {
            self->publishStatus();
            self->m_last_status_ms = now_ms;
            self->m_last_state = current_state;
        }
    }
}
