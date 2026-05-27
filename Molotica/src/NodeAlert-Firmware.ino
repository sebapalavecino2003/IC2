#include <Arduino.h>
#include "Config.h"
#include "Core.h"
#include "Sensors.h"
#include "MqttManager.h"
#include "Managers.h"
#include "Utils.h"

ThresholdConfig g_thresholds = {
    DEFAULT_TEMP_MIN, DEFAULT_TEMP_MAX,
    DEFAULT_HUM_MIN, DEFAULT_HUM_MAX,
    DEFAULT_GAS_THRESHOLD, DEFAULT_FLAME_THRESHOLD
};

StateMachine g_sm;
ErrorHandler g_err;
MessageBuffer g_msg_buf;
BuzzerManager g_buzzer;
WifiManager g_wifi;
SerialManager g_log;
DHT22Driver g_dht(DHT22_PIN);
MQ9Driver g_mq9(MQ9_PIN, MQ9_DIGITAL_PIN, MQ9_HEATER_PIN);
KY026Driver g_ky026(KY026_PIN, KY026_DIGITAL_PIN);
SensorCalibration g_cal;
SensorManager g_sensors(&g_dht, &g_mq9, &g_ky026, &g_cal, &g_msg_buf, &g_err, &g_log);
MqttManager g_mqtt(&g_sm, &g_err, &g_msg_buf);
AutomationManager g_automation(&g_sm, &g_msg_buf, &g_err, &g_mqtt, &g_buzzer, &g_log);

QueueHandle_t sensorDataQueue = NULL;
SemaphoreHandle_t configMutex = NULL;

void sensorTask(void* pvParams) {
    (void)pvParams;
    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        SensorReading reading = g_sensors.readAll();

        g_automation.evaluate(reading);
        g_log.printSensor(reading);

        if (sensorDataQueue != NULL) {
            xQueueSend(sensorDataQueue, &reading, 0);
        }

        g_buzzer.update();

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLING_INTERVAL_MS));
    }
}

void mqttTask(void* pvParams) {
    (void)pvParams;
    SensorReading reading;

    while (true) {
        g_wifi.update();
        g_mqtt.update();
        g_buzzer.update();

        if (sensorDataQueue != NULL) {
            if (xQueueReceive(sensorDataQueue, &reading, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_mqtt.publishSensorData(reading);
            }
        }

        taskYIELD();
    }
}

void setup() {
    g_log.init();
    g_log.printInfo("SYS", "NodeAlert IoT starting...");

    sensorDataQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorReading));
    configMutex = xSemaphoreCreateMutex();

    g_sm.setState(SystemState::INIT);
    g_buzzer.init();
    g_buzzer.beep(2000, 100);

    g_log.printInfo("WIFI", "Connecting to WiFi...");
    g_sm.setState(SystemState::WIFI_CONNECTING);
    g_wifi.init();
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RETRIES) {
        delay(WIFI_RETRY_MS);
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        g_sm.setState(SystemState::WIFI_CONNECTED);
        g_log.printInfo("WIFI", "Connected");
    } else {
        g_err.logError(ErrorCode::WIFI_FAIL, "WiFi connection failed");
        g_log.printError("WIFI", "Failed to connect");
    }

    g_log.printInfo("SENS", "Initializing sensors...");
    g_cal.loadCalibration();
    if (g_sensors.init()) {
        g_sensors.updateCalibration();
        g_sensors.calibrateAll();
        g_sm.setState(SystemState::SENSOR_READY);
        g_log.printInfo("SENS", "Sensors ready");
    } else {
        g_err.logError(ErrorCode::DHT_FAIL, "No sensors initialized");
        g_log.printError("SENS", "No sensors available");
    }

    g_log.printInfo("MQTT", "Setting up MQTT...");
    g_sm.setState(SystemState::MQTT_CONNECTING);
    g_mqtt.init();
    if (WiFi.status() == WL_CONNECTED) {
        if (g_mqtt.connect()) {
            g_sm.setState(SystemState::MQTT_CONNECTED);
            g_log.printInfo("MQTT", "Connected to broker");
        } else {
            g_err.logError(ErrorCode::MQTT_FAIL, "MQTT connection failed");
            g_log.printError("MQTT", "Failed to connect to broker");
        }
    }

    g_sm.setState(SystemState::MONITORING);
    g_log.printInfo("SYS", "NodeAlert IoT ready");

    xTaskCreatePinnedToCore(
        sensorTask, "sensorTask", SENSOR_TASK_STACK, NULL,
        SENSOR_TASK_PRIO, NULL, 1
    );

    xTaskCreatePinnedToCore(
        mqttTask, "mqttTask", MQTT_TASK_STACK, NULL,
        MQTT_TASK_PRIO, NULL, 1
    );

    g_log.printInfo("SYS", "FreeRTOS tasks created");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
