/**
 * @file sensor_manager.h
 * @brief Sensor orchestrator with FreeRTOS tasks, queues, and mutex
 *
 * The SensorManager owns all sensor driver instances and manages their
 * lifecycle via FreeRTOS tasks. Each sensor runs in its own task with
 * a priority appropriate to its criticality:
 *
 *   KY-026 (flame): priority 4 (critical) — 200ms interval
 *   MQ-9  (gas):    priority 3 (high)     — 1000ms interval
 *   DHT22 (temp):   priority 2 (normal)   — 2000ms interval
 *
 * A shared mutex (hw_mutex) protects GPIO/ADC access across tasks.
 * Readings are published to a single queue for consumption by the
 * main loop or other system components.
 */

#pragma once

#include "hal/sensor_reading.h"
#include "drivers/sensor/dht22_driver.h"
#include "drivers/sensor/mq9_driver.h"
#include "drivers/sensor/ky026_driver.h"
#include "drivers/sensor/calibration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <type_traits>

class SensorManager {
public:
    /**
     * @brief Construct an empty SensorManager (call init() before use)
     */
    SensorManager();

    /**
     * @brief Initialise all sensor drivers, mutex, and queue
     *
     * Creates the hw_mutex, sensor_queue, driver instances for all
     * three sensors, and SensorCalibration modules.
     */
    void init();

    /**
     * @brief Start all FreeRTOS sensor tasks
     *
     * Spawns three tasks with assigned priorities:
     *   - KY-026: priority 4 on core 1
     *   - MQ-9:   priority 3 on core 1
     *   - DHT22:  priority 2 on core 1
     */
    void startTasks();

    /**
     * @brief Get the last known reading for a sensor type
     * @param type The sensor type to query
     * @return Most recent SensorReading for that type
     */
    SensorReading getLatestReading(SensorType type) const;

    /**
     * @brief Get the queue handle for receiving sensor readings
     * @return xQueueHandle for the sensor reading queue
     */
    QueueHandle_t getReadingQueue() const { return sensor_queue; }

    TaskHandle_t getTaskHandle(SensorType type) const
    {
        switch (type) {
            case SensorType::DHT22_TEMPERATURE:
            case SensorType::DHT22_HUMIDITY:
                return task_dht22;
            case SensorType::MQ9_GAS:
                return task_mq9;
            case SensorType::KY026_FLAME:
                return task_ky026;
            default:
                return nullptr;
        }
    }

    /**
     * @brief Type-safe access to a specific sensor driver
     *
     * Usage: SensorManager* mgr = ...;
     *        DHT22Driver* dht = mgr->getSensor<DHT22Driver>();
     *
     * @tparam T The concrete driver class (DHT22Driver, MQ9Driver, KY026Driver)
     * @return Pointer to the driver, or nullptr if type is unsupported
     */
    template<typename T>
    T* getSensor()
    {
        if constexpr (std::is_same_v<T, DHT22Driver>)  return dht22;
        if constexpr (std::is_same_v<T, MQ9Driver>)     return mq9;
        if constexpr (std::is_same_v<T, KY026Driver>)   return ky026;
        return nullptr;
    }

private:
    // -- Sensor drivers -------------------------------------------------------
    DHT22Driver*    dht22;
    MQ9Driver*      mq9;
    KY026Driver*    ky026;

    // -- Calibration modules --------------------------------------------------
    SensorCalibration* cal_dht22;
    SensorCalibration* cal_mq9;
    SensorCalibration* cal_ky026;

    // -- RTOS primitives ------------------------------------------------------
    QueueHandle_t       sensor_queue;    ///< Reading queue (item = SensorReading, len = 20)
    SemaphoreHandle_t   hw_mutex;        ///< Mutex protecting shared GPIO/ADC access

    // -- Task handles ---------------------------------------------------------
    TaskHandle_t task_dht22;
    TaskHandle_t task_mq9;
    TaskHandle_t task_ky026;

    // -- Latest readings by type ----------------------------------------------
    SensorReading latest_readings[4];   ///< Indexed by SensorType (0-3)

    // -- FreeRTOS task functions (static) -------------------------------------
    static void taskDHT22(void* pvParams);
    static void taskMQ9(void* pvParams);
    static void taskKY026(void* pvParams);
};
