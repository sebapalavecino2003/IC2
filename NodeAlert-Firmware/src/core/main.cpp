/**
 * @file main.cpp
 * @brief NodeAlert IoT — Main firmware entry point (integrated)
 *
 * Full system integration with:
 *   - StateMachine (6 states, 10 transitions)
 *   - ErrorHandler (exponential backoff auto-recovery)
 *   - SensorManager (3 FreeRTOS sensor tasks with queue + mutex)
 *   - TaskManager (health monitor task)
 *
 * Initialisation sequence:
 *   NVS init → Serial init → StateMachine INIT →
 *   StateMachine STANDBY → SensorManager::startTasks() →
 *   StateMachine RUNNING → TaskManager::startMonitorTask() → main loop
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"

#include "config/pins_config.h"
#include "config/sampling_config.h"
#include "core/system_core.h"
#include "core/state_machine.h"
#include "core/error_handler.h"
#include "managers/sensor_manager.h"
#include "managers/task_manager.h"
#include "managers/automation_manager.h"
#include "managers/mqtt_manager.h"
#include "services/serial_manager.h"

extern "C" void app_main(void)
{
    /* ================================================================== */
    /* 1. Initialise NVS flash                                            */
    /* ================================================================== */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Enable Task WDT — panics (reboot) if any subscribed task doesn't reset within timeout */
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS, true);
    /* Interrupt WDT enabled via Kconfig: CONFIG_ESP_INT_WDT=y in sdkconfig */

    /* ================================================================== */
    /* 2. Initialise serial output                                        */
    /* ================================================================== */
    SerialManager::init(115200);
    SerialManager::printState("INIT");

    /* ================================================================== */
    /* 3. Create StateMachine — set alert thresholds                      */
    /* ================================================================== */
    StateMachine stateMachine;
    stateMachine.setAlertThresholds(TEMP_THRESHOLD_HIGH_C, GAS_THRESHOLD_HIGH);

    /* ================================================================== */
    /* 4. Create ErrorHandler                                             */
    /* ================================================================== */
    ErrorHandler errorHandler;

    /* ================================================================== */
    /* 5. Create SensorManager — initialise drivers, mutex, queue         */
    /* ================================================================== */
    SensorManager sensorManager;
    sensorManager.init();

    /* ================================================================== */
    /* 6. Create TaskManager — wire up system components                  */
    /* ================================================================== */
    TaskManager taskManager;
    taskManager.init(&sensorManager, &stateMachine, &errorHandler);

    /* ================================================================== */
    /* 7. Transition to STANDBY — init complete                           */
    /* ================================================================== */
    stateMachine.transitionTo(SystemState::STANDBY);

    /* ================================================================== */
    /* 8. Spawn all FreeRTOS sensor tasks                                 */
    /* ================================================================== */
    sensorManager.startTasks();
    esp_task_wdt_add(sensorManager.getTaskHandle(SensorType::DHT22_TEMPERATURE));
    esp_task_wdt_add(sensorManager.getTaskHandle(SensorType::MQ9_GAS));
    esp_task_wdt_add(sensorManager.getTaskHandle(SensorType::KY026_FLAME));

    /* ================================================================== */
    /* 9. Transition to RUNNING — system fully operational                */
    /* ================================================================== */
    stateMachine.transitionTo(SystemState::RUNNING);

    /* ================================================================== */
    /* 10. Start the health monitor task (lowest priority)                */
    /* ================================================================== */
    taskManager.startMonitorTask();
    esp_task_wdt_add(taskManager.getTaskHandle());

    /* ================================================================== */
    /* 11. Create AutomationManager — threshold evaluation & actuator      */
    /* ================================================================== */
    AutomationManager autoManager;
    autoManager.init(sensorManager.getReadingQueue(), &errorHandler);
    autoManager.startTask();
    esp_task_wdt_add(autoManager.getTaskHandle());

    /* ================================================================== */
    /* 12. Create MqttManager — MQTT telemetry publisher task              */
    /* ================================================================== */
    MqttManager mqttManager;
    mqttManager.init(sensorManager.getReadingQueue(), &errorHandler, &stateMachine);
    mqttManager.setAutoManager(&autoManager);
    mqttManager.startMqttTask();
    esp_task_wdt_add(mqttManager.getTaskHandle());
    esp_task_wdt_add(NULL);

    /* ================================================================== */
    /* 12. Main loop — queue consumer, alert/error/recovery management    */
    /* ================================================================== */
    QueueHandle_t queue = sensorManager.getReadingQueue();
    SensorReading reading;

    while (1) {
        esp_task_wdt_reset();
        // Receive next sensor reading from the queue (100ms timeout)
        if (xQueueReceive(queue, &reading, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Print the reading via SerialManager
            SerialManager::printReading(reading);

            // ---- Alert detection (RUNNING → ALERT) ----
            if (stateMachine.getCurrentState() == SystemState::RUNNING) {
                bool should_alert = false;

                switch (reading.type) {
                    case SensorType::DHT22_TEMPERATURE:
                        if (reading.value > stateMachine.getAlertTempHigh()) {
                            should_alert = true;
                        }
                        break;
                    case SensorType::MQ9_GAS:
                        if (reading.value > stateMachine.getAlertGasHigh()) {
                            should_alert = true;
                        }
                        break;
                    case SensorType::KY026_FLAME:
                        // Flame detected via interrupt — value > 0.5 means flame present
                        if (reading.value > 0.5f) {
                            should_alert = true;
                        }
                        break;
                    default:
                        break;
                }

                if (should_alert) {
                    errorHandler.reportError("System", "Alert condition detected");
                    stateMachine.transitionTo(SystemState::ALERT);
                }
            }

            // ---- Normalisation check (ALERT → RUNNING with hysteresis) ----
            if (stateMachine.getCurrentState() == SystemState::ALERT) {
                // Check all sensor thresholds with hysteresis
                SensorReading curr_temp = sensorManager.getLatestReading(
                    SensorType::DHT22_TEMPERATURE);
                SensorReading curr_gas  = sensorManager.getLatestReading(
                    SensorType::MQ9_GAS);
                SensorReading curr_flame = sensorManager.getLatestReading(
                    SensorType::KY026_FLAME);

                bool temp_ok   = (curr_temp.status  == SensorStatus::OK &&
                                  curr_temp.value   <= (stateMachine.getAlertTempHigh() -
                                                        HYSTERESIS_TEMP));
                bool gas_ok    = (curr_gas.status    == SensorStatus::OK &&
                                  curr_gas.value     <= (stateMachine.getAlertGasHigh() -
                                                        HYSTERESIS_GAS));
                bool flame_ok  = (curr_flame.status  == SensorStatus::OK &&
                                  curr_flame.value   <= 0.5f);

                if (temp_ok && gas_ok && flame_ok) {
                    errorHandler.clearError("System");
                    stateMachine.transitionTo(SystemState::RUNNING);
                }
            }

            // ---- Error detection (RUNNING → ERROR) ----
            if (reading.status != SensorStatus::OK &&
                stateMachine.getCurrentState() == SystemState::RUNNING) {
                errorHandler.reportError("Sensor", "Read failure");
                stateMachine.transitionTo(SystemState::ERROR);
            }
        }

        // ---- Recovery path (ERROR → RECOVERY → STANDBY → RUNNING) ----
        if (stateMachine.getCurrentState() == SystemState::ERROR &&
            errorHandler.shouldRecover()) {
            // Move to RECOVERY state
            stateMachine.transitionTo(SystemState::RECOVERY);
        }

        if (stateMachine.getCurrentState() == SystemState::RECOVERY) {
            // Attempt recovery: reset backoff, move through STANDBY to RUNNING
            errorHandler.resetBackoff();
            stateMachine.transitionTo(SystemState::STANDBY);
            stateMachine.transitionTo(SystemState::RUNNING);
        }

        // Yield to RTOS scheduler
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
