/**
 * @file ky026_driver.cpp
 * @brief KY-026 driver implementation — interrupt + ADC reading
 *
 * The KY-026 module has a comparator (LM393) that drives the digital
 * output high when IR radiation exceeds an adjustable threshold.
 * The analog output provides a voltage proportional to IR intensity.
 *
 * Interrupt configuration:
 *   - Digital pin: input with pull-down, rising-edge interrupt
 *   - ISR sets a volatile flag; the read() method consumes it
 *
 * ADC configuration:
 *   Uses the ESP-IDF adc_oneshot API via the shared ADC1 handle.
 *   12-bit resolution, 12 dB attenuation (0 – ~3.9 V input range).
 */

#include "ky026_driver.h"
#include "hal/adc_shared.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ========================================================================== */
/* Construction / destruction                                                 */
/* ========================================================================== */

KY026Driver::KY026Driver(gpio_num_t digital_pin, adc_channel_t analog_channel)
    : dig_pin(digital_pin)
    , ana_channel(analog_channel)
    , current_status(SensorStatus::OK)
    , flame_detected(false)
    , last_interrupt_ms(0)
{
    // ---- Configure digital pin as input with pull-down ----
    gpio_set_direction(dig_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(dig_pin, GPIO_PULLDOWN_ONLY);

    // ---- Configure interrupt: rising edge = flame detected ----
    gpio_set_intr_type(dig_pin, GPIO_INTR_POSEDGE);

    // ---- Install ISR service and register handler ----
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(dig_pin, interruptHandler, this);

    // ---- Configure the analog ADC channel ----
    adc_oneshot_unit_handle_t adc_handle;
    ESP_ERROR_CHECK(nodealert_adc1_init(&adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ana_channel, &chan_cfg));

    current_status = SensorStatus::OK;
}

KY026Driver::~KY026Driver()
{
    // Remove ISR handler and disable interrupt
    gpio_isr_handler_remove(dig_pin);
    gpio_set_intr_type(dig_pin, GPIO_INTR_DISABLE);
}

/* ========================================================================== */
/* ISR handler                                                                */
/* ========================================================================== */

void IRAM_ATTR KY026Driver::interruptHandler(void* arg)
{
    KY026Driver* driver = static_cast<KY026Driver*>(arg);
    driver->flame_detected    = true;
    driver->last_interrupt_ms = (uint32_t)xTaskGetTickCountFromISR();
}

/* ========================================================================== */
/* ISensor interface                                                          */
/* ========================================================================== */

SensorReading KY026Driver::read()
{
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Check interrupt flag first — highest priority
    if (flame_detected) {
        flame_detected = false;
        current_status = SensorStatus::OK;

        SensorReading r;
        r.type         = SensorType::KY026_FLAME;
        r.value        = 1.0f;         // flame present
        r.timestamp_ms = now_ms;
        r.status       = SensorStatus::OK;
        return r;
    }

    // Read digital pin for current state
    if (!readDigital()) {
        // No flame detected
        current_status = SensorStatus::OK;

        SensorReading r;
        r.type         = SensorType::KY026_FLAME;
        r.value        = 0.0f;         // no flame
        r.timestamp_ms = now_ms;
        r.status       = SensorStatus::OK;
        return r;
    }

    // Digital pin is high — flame present, read analog intensity
    int raw = readAnalog();

    current_status = SensorStatus::OK;

    SensorReading r;
    r.type         = SensorType::KY026_FLAME;
    r.value        = (float)raw / 4095.0f;   // normalised intensity
    r.timestamp_ms = now_ms;
    r.status       = SensorStatus::OK;
    return r;
}

bool KY026Driver::calibrate()
{
    // Read analog 5 times in known no-flame condition
    const int   num_samples = 5;
    float       sum = 0.0f;
    int         success_count = 0;

    for (int i = 0; i < num_samples; i++) {
        int raw = readAnalog();
        if (raw >= 0) {
            sum += (float)raw;
            success_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (success_count < 3) {
        current_status = SensorStatus::ERROR_READ_FAILURE;
        return false;
    }

    // Baseline ambient reading — just stored for reference
    float baseline = sum / (float)success_count;
    (void)baseline;   // available for future refinement

    current_status = SensorStatus::OK;
    return true;
}

SensorStatus KY026Driver::getStatus() const
{
    return current_status;
}

const char* KY026Driver::getName() const
{
    return "KY-026";
}

/* ========================================================================== */
/* Private helpers                                                            */
/* ========================================================================== */

bool KY026Driver::readDigital()
{
    return gpio_get_level(dig_pin) == 1;
}

int KY026Driver::readAnalog()
{
    adc_oneshot_unit_handle_t adc_handle;
    if (nodealert_adc1_init(&adc_handle) != ESP_OK) {
        return -1;
    }

    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, ana_channel, &raw);
    if (err != ESP_OK) {
        return -1;
    }

    return raw;
}
