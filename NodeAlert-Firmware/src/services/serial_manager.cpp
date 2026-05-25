/**
 * @file serial_manager.cpp
 * @brief Serial output service implementation
 *
 * All output is directed to the ESP-IDF console via printf(), which routes
 * to UART0 by default. Timestamps are derived from esp_timer_get_time()
 * providing millisecond-resolution uptime.
 */

#include "serial_manager.h"
#include "esp_timer.h"
#include <cstdio>

/* ========================================================================== */
/* Static helpers                                                             */
/* ========================================================================== */

uint32_t SerialManager::getTimestamp()
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

const char* SerialManager::sensorTypeToString(SensorType type)
{
    switch (type) {
        case SensorType::DHT22_TEMPERATURE: return "DHT22_TEMPERATURE";
        case SensorType::DHT22_HUMIDITY:    return "DHT22_HUMIDITY";
        case SensorType::MQ9_GAS:           return "MQ9_GAS";
        case SensorType::KY026_FLAME:       return "KY026_FLAME";
        default:                            return "UNKNOWN";
    }
}

const char* SerialManager::sensorStatusToString(SensorStatus status)
{
    switch (status) {
        case SensorStatus::OK:                  return "OK";
        case SensorStatus::ERROR_TIMEOUT:       return "ERROR_TIMEOUT";
        case SensorStatus::ERROR_READ_FAILURE:  return "ERROR_READ_FAILURE";
        case SensorStatus::ERROR_NOT_INITIALIZED: return "ERROR_NOT_INITIALIZED";
        case SensorStatus::ERROR_CALIBRATION:   return "ERROR_CALIBRATION";
        default:                                return "UNKNOWN";
    }
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

void SerialManager::init(int baud_rate)
{
    // ESP-IDF initialises UART0 as the default console before app_main().
    // The baud rate is configured via platformio.ini monitor_speed.
    (void)baud_rate;

    printf("[SYSTEM] NodeAlert IoT — Serial initialized\n");
    printf("[SYSTEM] Firmware v1.0 — System startup\n");
}

void SerialManager::printReading(const SensorReading& reading)
{
    printf("[%lu] SENSOR:%s VALUE:%.1f STATUS:%s\n",
           (unsigned long)getTimestamp(),
           sensorTypeToString(reading.type),
           (double)reading.value,
           sensorStatusToString(reading.status));
}

void SerialManager::printState(const char* state_name)
{
    printf("[%lu] STATE:%s\n",
           (unsigned long)getTimestamp(),
           (state_name != nullptr) ? state_name : "UNKNOWN");
}

void SerialManager::printError(const char* source, const char* message)
{
    printf("[%lu] ERROR:%s MSG:%s\n",
           (unsigned long)getTimestamp(),
           (source != nullptr) ? source : "UNKNOWN",
           (message != nullptr) ? message : "No details");
}
