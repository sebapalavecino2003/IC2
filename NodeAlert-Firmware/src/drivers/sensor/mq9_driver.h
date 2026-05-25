/**
 * @file mq9_driver.h
 * @brief MQ-9 gas sensor driver (CO and flammable gases)
 *
 * Implements the ISensor interface for the MQ-9 semiconductor gas
 * sensor. The sensor requires a 60-second preheating period before
 * producing valid readings. Output voltage varies with gas
 * concentration — the driver converts raw ADC readings to a
 * concentration ratio relative to the baseline (clean air).
 *
 * Gas estimation: ratio = (V_gas - V_clean) / V_clean * 100
 * A positive ratio indicates gas presence above baseline.
 *
 * The MQ-9 is sensitive to:
 *   - Carbon monoxide (CO)
 *   - Methane (CH4)
 *   - Liquefied Petroleum Gas (LPG)
 *
 * ADC interface:
 *   Uses the ESP-IDF adc_oneshot handle-based API (ESP-IDF 6.x).
 *   The shared ADC1 handle is obtained via nodealert_adc1_init().
 */

#pragma once

#include "hal/isensor.h"
#include "hal/sensor_reading.h"
#include "driver/gpio.h"
#include "hal/adc_types.h"         // adc_channel_t

class MQ9Driver : public ISensor {
public:
    /**
     * @brief Construct an MQ-9 driver on the specified ADC channel
     * @param adc_channel ADC1 channel connected to MQ-9 analog output
     * @param power_pin  Optional GPIO to control sensor power (set to
     *                   GPIO_NUM_NC if sensor is always powered)
     */
    explicit MQ9Driver(adc_channel_t adc_channel, gpio_num_t power_pin = GPIO_NUM_NC);

    /**
     * @brief Virtual destructor
     */
    ~MQ9Driver() override = default;

    // -- ISensor interface --------------------------------------------------
    SensorReading read() override;
    bool         calibrate() override;
    SensorStatus getStatus() const override;
    const char*  getName() const override;

private:
    adc_channel_t  channel;            ///< ADC1 channel for analog reading
    gpio_num_t     pwr_pin;            ///< Power control GPIO (NC if unused)
    SensorStatus   current_status;     ///< Last known sensor status
    bool           preheated;          ///< True after 60s preheat completes
    uint32_t       preheat_start_ms;   ///< Uptime when preheating began (ms)
    float          baseline_voltage;   ///< Clean-air voltage from calibration (V)
    float          last_reading;       ///< Last gas ratio reading (% change)
};
