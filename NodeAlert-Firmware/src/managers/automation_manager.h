#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "hal/thresholds.h"
#include "hal/sensor_reading.h"
#include "config/pins_config.h"

class ErrorHandler;

class AutomationManager {
public:
    AutomationManager();

    void init(QueueHandle_t sensor_queue, ErrorHandler* eh);
    void startTask();
    void processCommand(const char* command_json);
    void setMqttClient(esp_mqtt_client_handle_t client);

    TaskHandle_t getTaskHandle() const { return m_task_handle; }

private:
    QueueHandle_t             m_sensor_queue;
    ErrorHandler*             m_error_handler;
    TaskHandle_t              m_task_handle;
    esp_mqtt_client_handle_t  m_mqtt_client;
    bool                      m_override_active;
    bool                      m_override_actuator_on;
    bool                      m_alarm_acknowledged;
    uint32_t                  m_actuator_on_ms;
    SensorReading             m_latest[4];

    static void automationTask(void* pvParams);
    bool evaluateThresholds();
    void controlActuator(bool turn_on, uint32_t now_ms);
    void publishEvent(const char* type, const char* severity, const char* message);

    static constexpr uint32_t ACTUATOR_MIN_ON_MS = 120000;
};
