/**
 * @file main.cpp
 * @brief NodeAlert IoT — Main firmware entry point
 *
 * Initializes system components and starts the application.
 * This is the ESP-IDF entry point (app_main).
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void)
{
    printf("NodeAlert IoT — Firmware v1.0\n");
    printf("System initialized. Waiting for component startup...\n");

    // Components will be initialized in subsequent phases.
    // For now, just keep the system alive.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
