/**
 * @file task_manager.h
 * @brief Task orchestrator — monitor and health-check task
 *
 * The TaskManager provides a low-priority monitoring task that
 * periodically checks sensor health and escalates persistent
 * errors to the state machine.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>

// Forward declarations to avoid circular includes
class SensorManager;
class StateMachine;
class ErrorHandler;

class TaskManager {
public:
    /**
     * @brief Construct an empty TaskManager (call init() before use)
     */
    TaskManager();

    /**
     * @brief Initialise the TaskManager with references to system components
     *
     * @param sm        Pointer to the SensorManager
     * @param sm_state  Pointer to the StateMachine
     * @param eh        Pointer to the ErrorHandler
     */
    void init(SensorManager* sm, StateMachine* sm_state, ErrorHandler* eh);

    /**
     * @brief Start the low-priority monitor task
     *
     * Creates a task with priority 1 (lowest) that runs every 10 seconds
     * to check sensor statuses and escalate persistent errors.
     */
    void startMonitorTask();

    TaskHandle_t getTaskHandle() const { return task_handle; }

private:
    SensorManager*  sensor_mgr;         ///< Reference to sensor orchestrator
    StateMachine*   state_mach;         ///< Reference to system state machine
    ErrorHandler*   error_handler;      ///< Reference to error handler
    TaskHandle_t    task_handle;        ///< Handle for the monitor task

    /**
     * @brief FreeRTOS monitor task function (static)
     *
     * Runs at priority 1 (lowest). Every 10 seconds:
     *   - Checks all sensor statuses via SensorManager
     *   - Prints a health summary via SerialManager
     *   - If any sensor has been in error for > 30s, escalates to StateMachine
     *
     * @param pvParams Pointer to the TaskManager instance
     */
    static void monitorTask(void* pvParams);
};
