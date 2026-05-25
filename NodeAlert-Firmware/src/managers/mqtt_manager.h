#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "core/message_buffer.h"
#include <cstdint>

class ErrorHandler;
class StateMachine;

class MqttManager {
public:
    MqttManager();

    void init(QueueHandle_t sensor_queue, ErrorHandler* eh, StateMachine* sm);
    void startMqttTask();

private:
    QueueHandle_t           m_sensor_queue;
    ErrorHandler*           m_error_handler;
    StateMachine*           m_state_mach;
    TaskHandle_t            m_task_handle;
    esp_mqtt_client_handle_t m_client;
    bool                    m_connected;
    MessageBuffer           m_buffer;

    static void mqttTask(void* pvParams);
    static void mqttEventHandler(void* handler_args, esp_event_base_t base,
                                 int32_t event_id, void* event_data);

    void publishReading(const struct SensorReading& reading);
    void drainBuffer();
    void buildTelemetryJson(const struct SensorReading& reading,
                            char* buffer, size_t buffer_size);
    void buildTopic(const char* suffix, char* buffer, size_t buffer_size);
};
