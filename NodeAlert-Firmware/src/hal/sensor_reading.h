/**
 * @file sensor_reading.h
 * @brief Data structures for sensor readings in NodeAlert IoT
 *
 * Defines the common types used across all sensor drivers:
 * - SensorType: Identifies which sensor produced a reading
 * - SensorStatus: Result status of a sensor read operation
 * - SensorReading: Complete reading with type, value, timestamp, and status
 */

#pragma once

#include <cstdint>

/* ========================================================================== */
/* SensorType — Identifies the physical sensor and measurement                */
/* ========================================================================== */
enum class SensorType : uint8_t {
    DHT22_TEMPERATURE,  ///< Temperature reading from DHT22 (°C)
    DHT22_HUMIDITY,     ///< Relative humidity reading from DHT22 (%)
    MQ9_GAS,            ///< Gas concentration from MQ-9 (raw ADC)
    KY026_FLAME         ///< Flame detection from KY-026 (digital/analog)
};

/* ========================================================================== */
/* SensorStatus — Result of a sensor read operation                           */
/* ========================================================================== */
enum class SensorStatus : uint8_t {
    OK,                     ///< Reading successful
    ERROR_TIMEOUT,          ///< Sensor did not respond within timeout
    ERROR_READ_FAILURE,     ///< Data read failed (CRC, framing, etc.)
    ERROR_NOT_INITIALIZED,  ///< Sensor driver not yet initialized
    ERROR_CALIBRATION       ///< Sensor calibration required or failed
};

/* ========================================================================== */
/* SensorReading — Complete reading from any sensor                           */
/* ========================================================================== */
struct SensorReading {
    SensorType    type;          ///< Which sensor produced this reading
    float         value;         ///< Measured value (physical unit or raw ADC)
    uint32_t      timestamp_ms;  ///< System uptime when reading was taken (ms)
    SensorStatus  status;        ///< Success or error status

    /**
     * @brief Default constructor — initializes to safe defaults
     */
    SensorReading()
        : type(SensorType::DHT22_TEMPERATURE)
        , value(0.0f)
        , timestamp_ms(0)
        , status(SensorStatus::OK)
    {}
};
