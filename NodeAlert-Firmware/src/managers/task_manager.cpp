/**
 * @file task_manager.cpp
 * @brief Task manager implementation — health monitoring
 *
 * The monitor task runs at the lowest priority (1) and periodically
 * reports sensor health. If a sensor persists in an error state for
 * more than 30 seconds, the monitor escalates by transitioning the
 * state machine to ERROR and reporting the failure to the ErrorHandler.
 */

#include "task_manager.h"
#include "managers/sensor_manager.h"
#include "core/state_machine.h"
#include "core/error_handler.h"
#include "services/serial_manager.h"
#include "config/sampling_config.h"
#include "esp_timer.h"

/* ========================================================================== */
/* Construction / Initialisation                                               */
/* ========================================================================== */

TaskManager::TaskManager()
    : sensor_mgr(nullptr)
    , state_mach(nullptr)
    , error_handler(nullptr)
    , task_handle(nullptr)
{
}

void TaskManager::init(SensorManager* sm, StateMachine* sm_state, ErrorHandler* eh)
{
    sensor_mgr    = sm;
    state_mach    = sm_state;
    error_handler = eh;
}

void TaskManager::startMonitorTask()
{
    xTaskCreatePinnedToCore(monitorTask, "monitor_task", 3072, this, 1, &task_handle, 1);
}

/* ========================================================================== */
/* Monitor task — priority 1 (lowest), 10s interval                          */
/* ========================================================================== */

void TaskManager::monitorTask(void* pvParams)
{
    TaskManager* self = static_cast<TaskManager*>(pvParams);
    uint32_t error_start_time[4] = {0, 0, 0, 0};
    bool     error_active[4]     = {false, false, false, false};
    const uint32_t ESCALATION_MS = 30000u;

    while (1) {
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        bool any_error = false;

        // Check each sensor type via SensorManager
        for (int st = 0; st < 4; st++) {
            SensorType type = static_cast<SensorType>(st);
            SensorReading r = self->sensor_mgr->getLatestReading(type);

            if (r.status != SensorStatus::OK) {
                any_error = true;

                // Track error onset
                if (!error_active[st]) {
                    error_active[st]     = true;
                    error_start_time[st] = now_ms;
                }

                // Escalate if error persists > 30s
                if ((now_ms - error_start_time[st]) > ESCALATION_MS) {
                    if (self->state_mach->getCurrentState() != SystemState::ERROR) {
                        self->error_handler->reportError(
                            "Monitor",
                            "Sensor error persists > 30s — escalating");
                        self->state_mach->transitionTo(SystemState::ERROR);
                    }
                }
            } else {
                // Sensor OK — clear error tracking
                if (error_active[st]) {
                    error_active[st]     = false;
                    error_start_time[st] = 0;

                    // Notify error handler of recovery
                    self->error_handler->clearError(
                        (r.type == SensorType::DHT22_TEMPERATURE ||
                         r.type == SensorType::DHT22_HUMIDITY) ? "DHT22" :
                        (r.type == SensorType::MQ9_GAS)        ? "MQ-9"  :
                        (r.type == SensorType::KY026_FLAME)    ? "KY-026" : "Sensor");
                }
            }
        }

        // Print health summary
        SerialManager::printState(self->state_mach->getStateName());

        if (!any_error) {
            SerialManager::printReading(
                self->sensor_mgr->getLatestReading(SensorType::DHT22_TEMPERATURE));
            SerialManager::printReading(
                self->sensor_mgr->getLatestReading(SensorType::MQ9_GAS));
            SerialManager::printReading(
                self->sensor_mgr->getLatestReading(SensorType::KY026_FLAME));
        }

        // Sleep for 10 seconds
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
