/**
 * @file ky026_driver.h
 * @brief KY-026 flame sensor driver with hardware interrupt support
 *
 * Implements the ISensor interface for the KY-026 flame/IR sensor.
 * The sensor provides both a digital output (flame threshold) and an
 * analog output (flame intensity).
 *
 * Hardware interrupt:
 *   The digital pin is configured for rising-edge interrupt. When a
 *   flame is detected (digital pin goes HIGH), the ISR sets an atomic
 *   flag. The next call to read() returns 1.0f and resets the flag.
 *
 * Analog reading:
 *   The analog pin reads flame intensity via the ESP-IDF adc_oneshot
 *   API (ESP-IDF 6.x). The shared ADC1 handle is obtained via
 *   nodealert_adc1_init(). Values range from 0 (no flame) to 4095
 *   (maximum detected).
 */

#pragma once

#include "hal/isensor.h"
#include "hal/sensor_reading.h"
#include "hal/adc_types.h"          // adc_channel_t
#include "driver/gpio.h"

class KY026Driver : public ISensor {
public:
    /**
     * @brief Construct a KY-026 driver
     * @param digital_pin   GPIO pin for digital output (interrupt capable)
     * @param analog_channel ADC1 channel for analog intensity reading
     */
    explicit KY026Driver(gpio_num_t digital_pin, adc_channel_t analog_channel);

    /**
     * @brief Virtual destructor — removes ISR handler
     */
    ~KY026Driver() override;

    // -- ISensor interface --------------------------------------------------
    SensorReading read() override;
    bool         calibrate() override;
    SensorStatus getStatus() const override;
    const char*  getName() const override;

    /**
     * @brief ISR handler (static, IRAM)
     *
     * Called on rising edge of the digital pin. Only writes to
     * volatile member variables. The argument is a pointer to the
     * KY026Driver instance.
     */
    static void IRAM_ATTR interruptHandler(void* arg);

private:
    gpio_num_t      dig_pin;            ///< Digital output GPIO
    adc_channel_t   ana_channel;        ///< Analog intensity ADC channel
    SensorStatus    current_status;     ///< Last known sensor status

    volatile bool   flame_detected;     ///< Set true by ISR on rising edge
    uint32_t        last_interrupt_ms;  ///< Tick count at last interrupt

    /**
     * @brief Read the digital output pin
     * @return true if pin is high (flame detected), false otherwise
     */
    bool readDigital();

    /**
     * @brief Read the analog intensity value
     * @return Raw ADC reading (0–4095)
     */
    int readAnalog();
};
