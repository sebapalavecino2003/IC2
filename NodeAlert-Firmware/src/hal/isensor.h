/**
 * @file isensor.h
 * @brief Abstract sensor interface (Hardware Abstraction Layer)
 *
 * All sensor drivers implement this interface. The application code
 * works with ISensor pointers, never with concrete driver classes
 * directly. This decouples business logic from hardware specifics.
 */

#pragma once

#include "hal/sensor_reading.h"

/* ========================================================================== */
/* ISensor — Pure virtual interface for all sensor drivers                    */
/* ========================================================================== */
class ISensor {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~ISensor() = default;

    /**
     * @brief Take a synchronous reading from the sensor
     * @return SensorReading struct with type, value, timestamp, status
     */
    virtual SensorReading read() = 0;

    /**
     * @brief Perform sensor calibration (if supported)
     * @return true if calibration succeeded, false otherwise
     */
    virtual bool calibrate() = 0;

    /**
     * @brief Get the current sensor status
     * @return SensorStatus enum value
     */
    virtual SensorStatus getStatus() const = 0;

    /**
     * @brief Get the human-readable sensor name
     * @return Pointer to a C-string with the sensor name
     */
    virtual const char* getName() const = 0;
};
