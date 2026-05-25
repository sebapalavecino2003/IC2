/**
 * @file mq9_driver.cpp
 * @brief MQ-9 driver implementation — ADC reading, preheating, calibration
 *
 * The MQ-9 is a semiconductor gas sensor whose conductivity changes
 * with gas concentration. It requires a 60-second warm-up (heater
 * stabilisation) before readings are valid.
 *
 * Voltage-to-concentration is non-linear; the driver outputs a ratio
 * (percentage change from baseline) that can be compared against
 * thresholds defined in sampling_config.h.
 *
 * ADC configuration uses the ESP-IDF adc_oneshot API with 12-bit
 * resolution and 12 dB attenuation (input range 0 – ~3.9 V).
 */

#include "mq9_driver.h"
#include "hal/adc_shared.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ========================================================================== */
/* Construction                                                               */
/* ========================================================================== */

MQ9Driver::MQ9Driver(adc_channel_t adc_channel, gpio_num_t power_pin)
    : channel(adc_channel)
    , pwr_pin(power_pin)
    , current_status(SensorStatus::OK)
    , preheated(false)
    , preheat_start_ms(0)
    , baseline_voltage(0.0f)
    , last_reading(0.0f)
{
    // Obtain the shared ADC1 handle
    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(nodealert_adc1_init(&adc_handle));

    // Configure this channel: 12-bit width, 12 dB attenuation
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &chan_cfg));
}

/* ========================================================================== */
/* ISensor interface                                                          */
/* ========================================================================== */

bool MQ9Driver::calibrate()
{
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Power on the sensor if a control pin is configured
    if (pwr_pin != GPIO_NUM_NC) {
        gpio_set_direction(pwr_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(pwr_pin, 1);
    }

    // Begin 60-second preheat
    preheated        = false;
    preheat_start_ms = now_ms;

    // Obtain the shared ADC1 handle for reading
    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(nodealert_adc1_init(&adc_handle));

    // Wait for heater to stabilise (60 s)
    vTaskDelay(pdMS_TO_TICKS(60000));

    // Read 10 ADC samples for baseline
    const int   num_samples = 10;
    float       samples[num_samples];
    float       sum = 0.0f;

    for (int i = 0; i < num_samples; i++) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(adc_handle, channel, &raw);
        if (err != ESP_OK) {
            current_status = SensorStatus::ERROR_READ_FAILURE;
            return false;
        }
        samples[i] = (float)raw * 3.3f / 4095.0f;   // convert to voltage
        sum += samples[i];
        vTaskDelay(pdMS_TO_TICKS(100));               // 100 ms between samples
    }

    float mean = sum / (float)num_samples;

    // Verify readings are within 10 % variance
    float max_dev = 0.0f;
    for (int i = 0; i < num_samples; i++) {
        float dev = (samples[i] - mean) / mean;
        if (dev < 0.0f) dev = -dev;
        if (dev > max_dev) max_dev = dev;
    }

    if (max_dev > 0.10f) {
        // Readings too unstable — calibration failed
        current_status = SensorStatus::ERROR_CALIBRATION;
        return false;
    }

    baseline_voltage = mean;
    preheated        = true;
    current_status   = SensorStatus::OK;
    return true;
}

SensorReading MQ9Driver::read()
{
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Reject read if preheat is still in progress
    if (!preheated && preheat_start_ms > 0 &&
        now_ms < preheat_start_ms + 60000) {
        current_status = SensorStatus::ERROR_CALIBRATION;

        SensorReading r;
        r.type         = SensorType::MQ9_GAS;
        r.value        = last_reading;
        r.timestamp_ms = now_ms;
        r.status       = SensorStatus::ERROR_CALIBRATION;
        return r;
    }

    // Obtain the shared ADC1 handle
    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(nodealert_adc1_init(&adc_handle));

    // Read raw ADC value
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, channel, &raw);
    if (err != ESP_OK) {
        current_status = SensorStatus::ERROR_READ_FAILURE;

        SensorReading r;
        r.type         = SensorType::MQ9_GAS;
        r.value        = last_reading;
        r.timestamp_ms = now_ms;
        r.status       = SensorStatus::ERROR_READ_FAILURE;
        return r;
    }

    // Convert to voltage
    float voltage = (float)raw * 3.3f / 4095.0f;
    last_reading = voltage;

    // Estimate gas concentration ratio (% change from baseline)
    float ratio = 0.0f;
    if (baseline_voltage > 0.0f) {
        ratio = (voltage - baseline_voltage) / baseline_voltage * 100.0f;
    }

    current_status = SensorStatus::OK;

    SensorReading r;
    r.type         = SensorType::MQ9_GAS;
    r.value        = ratio;
    r.timestamp_ms = now_ms;
    r.status       = SensorStatus::OK;
    return r;
}

SensorStatus MQ9Driver::getStatus() const
{
    return current_status;
}

const char* MQ9Driver::getName() const
{
    return "MQ-9";
}
