/**
 * @file sensor_manager.cpp
 * @brief Sensor manager implementation — FreeRTOS task orchestration
 *
 * Each sensor task runs in a tight loop: acquire the hw_mutex, read the
 * sensor, release the mutex, update the calibration module, push the
 * reading onto the shared queue, then sleep for the configured interval.
 *
 * The hw_mutex serialises access to shared hardware resources (GPIO
 * lines, the ADC1 unit) so that concurrent reads from different tasks
 * do not interfere.
 */

#include "sensor_manager.h"
#include "config/pins_config.h"
#include "config/sampling_config.h"
#include "hal/adc_shared.h"

/* ========================================================================== */
/* Construction / Initialisation                                               */
/* ========================================================================== */

SensorManager::SensorManager()
    : dht22(nullptr)
    , mq9(nullptr)
    , ky026(nullptr)
    , cal_dht22(nullptr)
    , cal_mq9(nullptr)
    , cal_ky026(nullptr)
    , sensor_queue(nullptr)
    , hw_mutex(nullptr)
    , task_dht22(nullptr)
    , task_mq9(nullptr)
    , task_ky026(nullptr)
{
    // Initialise latest_readings array to default-constructed SensorReading
    for (int i = 0; i < 4; i++) {
        latest_readings[i] = SensorReading();
    }
}

void SensorManager::init()
{
    // ---- Create the shared hardware mutex ----
    hw_mutex = xSemaphoreCreateMutex();
    if (hw_mutex == nullptr) {
        // System-level failure — can't proceed safely
        return;
    }

    // ---- Create the sensor reading queue ----
    sensor_queue = xQueueCreate(20, sizeof(SensorReading));
    if (sensor_queue == nullptr) {
        return;
    }

    // ---- Instantiate sensor drivers ----
    dht22 = new DHT22Driver(PIN_DHT22_DATA);
    mq9   = new MQ9Driver(PIN_MQ9_ADC_CHANNEL, GPIO_NUM_18);
    ky026 = new KY026Driver(PIN_KY026_DIGITAL, PIN_KY026_ADC_CHANNEL);

    // ---- Create calibration modules ----
    cal_dht22 = new SensorCalibration(CALIBRATION_SAMPLES);
    cal_mq9   = new SensorCalibration(CALIBRATION_SAMPLES);
    cal_ky026 = new SensorCalibration(CALIBRATION_SAMPLES);

    // ---- Initialise ADC shared handle (used by MQ-9 and KY-026) ----
    adc_oneshot_unit_handle_t adc_handle;
    nodealert_adc1_init(&adc_handle);

    // ---- Calibrate all sensors ----
    dht22->calibrate();
    mq9->calibrate();
    ky026->calibrate();
}

/* ========================================================================== */
/* Task creation                                                              */
/* ========================================================================== */

void SensorManager::startTasks()
{
    // KY-026: priority 4 (critical) — fastest response to flame events
    // MQ-9:   priority 3 (high)
    // DHT22:  priority 2 (normal)
    // All pinned to core 1 for balanced load vs. WiFi/FreeRTOS on core 0
    xTaskCreatePinnedToCore(taskDHT22, "dht22_task", 4096, this, 2, &task_dht22, 1);
    xTaskCreatePinnedToCore(taskMQ9,   "mq9_task",   4096, this, 3, &task_mq9,   1);
    xTaskCreatePinnedToCore(taskKY026, "ky026_task", 2048, this, 4, &task_ky026, 1);
}

/* ========================================================================== */
/* Sensor reading access                                                      */
/* ========================================================================== */

SensorReading SensorManager::getLatestReading(SensorType type) const
{
    size_t idx = static_cast<size_t>(type);
    if (idx < 4) {
        return latest_readings[idx];
    }
    return SensorReading();
}

/* ========================================================================== */
/* FreeRTOS task — DHT22 (priority 2, 2000ms interval)                       */
/* ========================================================================== */

void SensorManager::taskDHT22(void* pvParams)
{
    SensorManager* self = static_cast<SensorManager*>(pvParams);

    while (1) {
        // Acquire hardware mutex
        if (xSemaphoreTake(self->hw_mutex, portMAX_DELAY) == pdTRUE) {
            SensorReading reading = self->dht22->read();
            xSemaphoreGive(self->hw_mutex);

            // Add to calibration
            self->cal_dht22->addSample(reading.value);

            // Store as latest reading for this type
            size_t idx = static_cast<size_t>(reading.type);
            if (idx < 4) {
                self->latest_readings[idx] = reading;
            }

            // Push onto queue for consumers
            xQueueSend(self->sensor_queue, &reading, portMAX_DELAY);
        }

        // Sleep for the sampling interval
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_DHT22_MS));
    }
}

/* ========================================================================== */
/* FreeRTOS task — MQ-9 (priority 3, 1000ms interval)                        */
/* ========================================================================== */

void SensorManager::taskMQ9(void* pvParams)
{
    SensorManager* self = static_cast<SensorManager*>(pvParams);

    while (1) {
        // Acquire hardware mutex
        if (xSemaphoreTake(self->hw_mutex, portMAX_DELAY) == pdTRUE) {
            SensorReading reading = self->mq9->read();
            xSemaphoreGive(self->hw_mutex);

            // Add to calibration
            self->cal_mq9->addSample(reading.value);

            // Store as latest reading for this type
            size_t idx = static_cast<size_t>(reading.type);
            if (idx < 4) {
                self->latest_readings[idx] = reading;
            }

            // Push onto queue for consumers
            xQueueSend(self->sensor_queue, &reading, portMAX_DELAY);
        }

        // Sleep for the sampling interval
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MQ9_MS));
    }
}

/* ========================================================================== */
/* FreeRTOS task — KY-026 (priority 4, 200ms interval)                       */
/* ========================================================================== */

void SensorManager::taskKY026(void* pvParams)
{
    SensorManager* self = static_cast<SensorManager*>(pvParams);

    while (1) {
        // Acquire hardware mutex
        if (xSemaphoreTake(self->hw_mutex, portMAX_DELAY) == pdTRUE) {
            SensorReading reading = self->ky026->read();
            xSemaphoreGive(self->hw_mutex);

            // Add to calibration
            self->cal_ky026->addSample(reading.value);

            // Store as latest reading for this type
            size_t idx = static_cast<size_t>(reading.type);
            if (idx < 4) {
                self->latest_readings[idx] = reading;
            }

            // Push onto queue for consumers
            xQueueSend(self->sensor_queue, &reading, portMAX_DELAY);
        }

        // Sleep for the sampling interval
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_KY026_MS));
    }
}
