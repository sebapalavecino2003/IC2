/**
 * @file main.cpp
 * @brief NodeAlert IoT — Main firmware entry point
 *
 * Implements the INIT → RUNNING state machine with DHT22 sensor sampling
 * and formatted serial output. The system:
 *   1. Initialises NVS flash for persistent storage
 *   2. Configures serial output via SerialManager
 *   3. Creates the DHT22 temperature/humidity driver
 *   4. Transitions INIT → RUNNING
 *   5. Samples the DHT22 every SAMPLE_INTERVAL_DHT22_MS (2000ms)
 *   6. Outputs formatted readings via serial
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "config/pins_config.h"
#include "config/sampling_config.h"
#include "core/system_core.h"
#include "drivers/sensor/dht22_driver.h"
#include "services/serial_manager.h"

extern "C" void app_main(void)
{
    /* ---- Step 1: Initialise NVS flash ---- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated or upgraded — erase and retry
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- Step 2: Initialise serial output ---- */
    SerialManager::init(115200);

    /* ---- Step 3: Print INIT state ---- */
    SerialManager::printState(stateToString(SystemState::INIT));

    /* ---- Step 4: Create DHT22 sensor driver ---- */
    DHT22Driver dht22(PIN_DHT22_DATA);

    /* ---- Step 5: Transition to RUNNING ---- */
    SerialManager::printState(stateToString(SystemState::RUNNING));
    SystemState current_state = SystemState::RUNNING;

    /* ---- Step 6: Main sampling loop ---- */
    while (1) {
        // Read the DHT22 sensor (temperature + humidity)
        SensorReading reading = dht22.read();

        // Print the reading
        SerialManager::printReading(reading);

        // Print current system state
        SerialManager::printState(stateToString(current_state));

        // Basic error handling
        if (reading.status != SensorStatus::OK) {
            SerialManager::printError(dht22.getName(), "Read failure — using cached value");
        }

        // Wait for next sampling interval
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_DHT22_MS));
    }
}
