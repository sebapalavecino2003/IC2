#include "automation_manager.h"
#include "core/error_handler.h"
#include "config/mqtt_broker_config.h"
#include "esp_timer.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

AutomationManager::AutomationManager()
    : m_sensor_queue(nullptr)
    , m_error_handler(nullptr)
    , m_task_handle(nullptr)
    , m_mqtt_client(nullptr)
    , m_override_active(false)
    , m_override_actuator_on(false)
    , m_alarm_acknowledged(false)
    , m_actuator_on_ms(0)
{
    for (int i = 0; i < 4; i++) {
        m_latest[i] = SensorReading();
    }
}

void AutomationManager::init(QueueHandle_t sensor_queue, ErrorHandler* eh)
{
    m_sensor_queue  = sensor_queue;
    m_error_handler = eh;
}

void AutomationManager::startTask()
{
    xTaskCreatePinnedToCore(automationTask, "auto_task", 4096, this, 2, &m_task_handle, 1);
}

void AutomationManager::setMqttClient(esp_mqtt_client_handle_t client)
{
    m_mqtt_client = client;
}

void AutomationManager::automationTask(void* pvParams)
{
    AutomationManager* self = static_cast<AutomationManager*>(pvParams);
    SensorReading reading;
    bool     alert_active      = false;
    uint32_t alert_entry_time  = 0;
    uint32_t actuator_on_time  = 0;

    gpio_reset_pin(PIN_RELAY_ACTUATOR);
    gpio_set_direction(PIN_RELAY_ACTUATOR, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELAY_ACTUATOR, 0);

    while (1) {
        while (xQueueReceive(self->m_sensor_queue, &reading, pdMS_TO_TICKS(0)) == pdTRUE) {
            int idx = static_cast<int>(reading.type);
            if (idx >= 0 && idx < 4) {
                self->m_latest[idx] = reading;
            }
        }

        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

        if (!self->m_override_active) {
            bool any_critical = self->evaluateThresholds();

            if (any_critical && !alert_active) {
                alert_active      = true;
                alert_entry_time  = now_ms;
                actuator_on_time  = now_ms;
                self->controlActuator(true, now_ms);
                self->publishEvent("threshold_crossed", "critical",
                    "Umbral crítico superado — actuador activado");
            }

            if (!any_critical && alert_active) {
                uint32_t elapsed_alert = now_ms - alert_entry_time;
                bool time_condition = elapsed_alert >= g_thresholds.hysteresis_time_ms;

                bool delta_condition = true;
                for (int i = 0; i < 4; i++) {
                    const auto& r = self->m_latest[i];
                    if (r.status != SensorStatus::OK) continue;

                    switch (r.type) {
                        case SensorType::DHT22_TEMPERATURE:
                            if (r.value >= g_thresholds.temp_warning * (1.0f - g_thresholds.hysteresis_delta_pct)) {
                                delta_condition = false;
                            }
                            break;
                        case SensorType::DHT22_HUMIDITY:
                            if (r.value <= g_thresholds.humidity_low * (1.0f + g_thresholds.hysteresis_delta_pct) &&
                                r.value >= g_thresholds.humidity_high * (1.0f - g_thresholds.hysteresis_delta_pct)) {
                            }
                            break;
                        case SensorType::MQ9_GAS:
                            if (r.value >= g_thresholds.gas_warning * (1.0f - g_thresholds.hysteresis_delta_pct)) {
                                delta_condition = false;
                            }
                            break;
                        case SensorType::KY026_FLAME:
                            if (r.value >= g_thresholds.flame_threshold * 0.9f) {
                                delta_condition = false;
                            }
                            break;
                    }
                }

                if (time_condition && delta_condition) {
                    alert_active = false;
                    uint32_t elapsed_actuator = now_ms - actuator_on_time;
                    if (elapsed_actuator >= ACTUATOR_MIN_ON_MS) {
                        self->controlActuator(false, now_ms);
                        self->publishEvent("actuator_off", "info",
                            "Condiciones normales — actuador desactivado");
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

bool AutomationManager::evaluateThresholds()
{
    bool critical = false;

    for (int i = 0; i < 4; i++) {
        const auto& r = m_latest[i];
        if (r.status != SensorStatus::OK) continue;

        switch (r.type) {
            case SensorType::DHT22_TEMPERATURE:
                if (r.value >= g_thresholds.temp_critical) {
                    critical = true;
                }
                break;

            case SensorType::DHT22_HUMIDITY:
                if (r.value <= g_thresholds.humidity_low ||
                    r.value >= g_thresholds.humidity_high) {
                }
                break;

            case SensorType::MQ9_GAS:
                if (r.value >= g_thresholds.gas_critical) {
                    critical = true;
                }
                break;

            case SensorType::KY026_FLAME:
                if (r.value >= g_thresholds.flame_threshold) {
                    critical = true;
                }
                break;
        }
    }

    return critical;
}

void AutomationManager::controlActuator(bool turn_on, uint32_t now_ms)
{
    if (turn_on) {
        gpio_set_level(PIN_RELAY_ACTUATOR, 1);
        m_actuator_on_ms = now_ms;
        publishEvent("actuator_on", "warning", "Actuador activado");
    } else {
        if (!m_override_active) {
            if ((now_ms - m_actuator_on_ms) < ACTUATOR_MIN_ON_MS) {
                return;
            }
        }
        gpio_set_level(PIN_RELAY_ACTUATOR, 0);
    }
}

void AutomationManager::publishEvent(const char* type, const char* severity, const char* message)
{
    if (!m_mqtt_client) return;

    char topic[64];
    char payload[192];

    snprintf(topic, sizeof(topic), "%s%s/events", MQTT_TOPIC_PREFIX, DEVICE_ID);

    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"event_type\":\"%s\",\"severity\":\"%s\","
        "\"message\":\"%s\",\"timestamp\":%lu}",
        DEVICE_ID, type, severity, message,
        (unsigned long)(esp_timer_get_time() / 1000));

    esp_mqtt_client_publish(m_mqtt_client, topic, payload, 0, 1, 0);
}

void AutomationManager::processCommand(const char* command_json)
{
    if (!command_json) return;

    if (strstr(command_json, "\"cmd\":\"actuator_on\"") ||
        strstr(command_json, "\"cmd\": \"actuator_on\"")) {
        m_override_active = true;
        m_override_actuator_on = true;
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        controlActuator(true, now_ms);
        publishEvent("override", "warning", "Override: Actuador ON forzado");
        return;
    }

    if (strstr(command_json, "\"cmd\":\"actuator_off\"") ||
        strstr(command_json, "\"cmd\": \"actuator_off\"")) {
        m_override_active = true;
        m_override_actuator_on = false;
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        controlActuator(false, now_ms);
        publishEvent("override", "warning", "Override: Actuador OFF forzado");
        return;
    }

    if (strstr(command_json, "\"cmd\":\"return_to_auto\"") ||
        strstr(command_json, "\"cmd\": \"return_to_auto\"")) {
        m_override_active = false;
        publishEvent("override", "info", "Override: Retorno a modo Auto");
        return;
    }

    if (strstr(command_json, "\"cmd\":\"acknowledge_alarm\"") ||
        strstr(command_json, "\"cmd\": \"acknowledge_alarm\"")) {
        m_alarm_acknowledged = true;
        publishEvent("override", "info", "Alarma silenciada");
        return;
    }

    if (strstr(command_json, "\"cmd\":\"update_thresholds\"") ||
        strstr(command_json, "\"cmd\": \"update_thresholds\"")) {
        const char* tc = strstr(command_json, "\"temp_critical\":");
        if (tc) {
            tc += 16;
            g_thresholds.temp_critical = (float)atof(tc);
        }
        const char* tw = strstr(command_json, "\"temp_warning\":");
        if (tw) {
            tw += 15;
            g_thresholds.temp_warning = (float)atof(tw);
        }
        const char* gc = strstr(command_json, "\"gas_critical\":");
        if (gc) {
            gc += 14;
            g_thresholds.gas_critical = (float)atof(gc);
        }
        const char* gw = strstr(command_json, "\"gas_warning\":");
        if (gw) {
            gw += 13;
            g_thresholds.gas_warning = (float)atof(gw);
        }
        const char* ft = strstr(command_json, "\"flame_threshold\":");
        if (ft) {
            ft += 17;
            g_thresholds.flame_threshold = (float)atof(ft);
        }

        publishEvent("thresholds", "info", "Umbrales actualizados vía MQTT");
    }
}
