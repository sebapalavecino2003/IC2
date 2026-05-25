/**
 * @file dht22_driver.h
 * @brief DHT22 temperature and humidity sensor driver
 *
 * Implements the ISensor interface for the DHT22 sensor using a
 * bit-banged one-wire protocol with microsecond timing via ESP-IDF.
 * The DHT22 provides both temperature (°C) and relative humidity (%)
 * in a single 40-bit data frame.
 *
 * Timing is critical (±10us tolerance); this driver uses ets_delay_us()
 * for active delays and esp_timer_get_time() for timeout measurements.
 */

#pragma once

#include "hal/isensor.h"
#include "hal/sensor_reading.h"
#include "driver/gpio.h"

class DHT22Driver : public ISensor {
public:
    /**
     * @brief Construct a DHT22 driver on the specified GPIO pin
     * @param pin GPIO number connected to the DHT22 data line
     */
    explicit DHT22Driver(gpio_num_t pin);

    /**
     * @brief Virtual destructor
     */
    ~DHT22Driver() override = default;

    // -- ISensor interface --------------------------------------------------
    SensorReading read() override;
    bool         calibrate() override;
    SensorStatus getStatus() const override;
    const char*  getName() const override;

    // -- DHT22-specific accessors -------------------------------------------
    float getLastHumidity() const { return last_humidity; }

private:
    gpio_num_t      data_pin;           ///< GPIO pin for DHT22 data line
    SensorStatus    current_status;     ///< Last known sensor status
    uint32_t        last_read_ms;       ///< System uptime of last successful read (ms)
    float           last_temperature;   ///< Last valid temperature (°C)
    float           last_humidity;      ///< Last valid relative humidity (%)

    /**
     * @brief Execute the DHT22 one-wire protocol
     *
     * Steps performed:
     *   1. Pull data line LOW for at least 18ms (start signal)
     *   2. Pull HIGH for 40us
     *   3. Release line and wait for sensor response
     *   4. Read 40 bits (16 humidity, 16 temperature, 8 checksum)
     *
     * @param[out] temperature Parsed temperature in °C
     * @param[out] humidity    Parsed relative humidity in %
     * @return true if the read succeeded and checksum validated
     */
    bool readRaw(float& temperature, float& humidity);
};
