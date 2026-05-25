/**
 * @file serial_manager.h
 * @brief Serial output service for formatted console output
 *
 * Provides static methods for printing sensor readings, state transitions,
 * and error messages with a consistent timestamped format.
 * Uses ESP-IDF's stdio console (UART via printf) at the configured baud rate.
 */

#pragma once

#include "hal/sensor_reading.h"
#include <cstdint>

class SerialManager {
public:
    /**
     * @brief Initialize serial console output
     * @param baud_rate UART baud rate (typically 115200 for NodeAlert)
     *
     * ESP-IDF initializes UART0 by default. This method ensures the console
     * is configured at the requested baud rate and prints a startup banner.
     */
    static void init(int baud_rate);

    /**
     * @brief Print a formatted sensor reading
     *
     * Format: [timestamp] SENSOR:{type} VALUE:{value} STATUS:{status}
     * Example: [1000] SENSOR:DHT22_TEMPERATURE VALUE:25.3 STATUS:OK
     *
     * @param reading The SensorReading to print
     */
    static void printReading(const SensorReading& reading);

    /**
     * @brief Print a system state transition notification
     *
     * Format: [timestamp] STATE:{state_name}
     * Example: [1000] STATE:RUNNING
     *
     * @param state_name Human-readable state name string
     */
    static void printState(const char* state_name);

    /**
     * @brief Print an error message from a system component
     *
     * Format: [timestamp] ERROR:{source} MSG:{message}
     * Example: [1000] ERROR:DHT22 MSG:Sensor timeout
     *
     * @param source  The component or driver that generated the error
     * @param message Descriptive error message
     */
    static void printError(const char* source, const char* message);

private:
    /**
     * @brief Get the current system uptime in milliseconds
     * @return Milliseconds since boot
     */
    static uint32_t getTimestamp();

    /**
     * @brief Convert SensorType enum to human-readable string
     */
    static const char* sensorTypeToString(SensorType type);

    /**
     * @brief Convert SensorStatus enum to human-readable string
     */
    static const char* sensorStatusToString(SensorStatus status);
};
