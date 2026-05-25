/**
 * @file dht22_driver.cpp
 * @brief DHT22 driver implementation — one-wire protocol and ISensor methods
 *
 * The DHT22 uses a single-wire bidirectional protocol. The host (ESP32)
 * initiates the communication by pulling the line low for at least 18ms,
 * then releasing it. The sensor responds by pulling low for 80us and
 * high for 80us, followed by 40 data bits (MSB-first).
 *
 * Data format (40 bits):
 *   [16 bits humidity] [16 bits temperature] [8 bits checksum]
 *
 * Where checksum = low 8 bits of (humidity_msb + humidity_lsb + temp_msb + temp_lsb)
 *
 * Temperature bit 15 indicates sign (0 = positive, 1 = negative).
 */

#include "dht22_driver.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"    // esp_rom_delay_us()
#include <cstring>

/* ========================================================================== */
/* Construction / destruction                                                 */
/* ========================================================================== */

DHT22Driver::DHT22Driver(gpio_num_t pin)
    : data_pin(pin)
    , current_status(SensorStatus::OK)
    , last_read_ms(0)
    , last_temperature(0.0f)
    , last_humidity(0.0f)
{
}

/* ========================================================================== */
/* ISensor interface                                                          */
/* ========================================================================== */

SensorReading DHT22Driver::read()
{
    float temperature = 0.0f;
    float humidity    = 0.0f;

    if (readRaw(temperature, humidity)) {
        last_temperature = temperature;
        last_humidity    = humidity;
        last_read_ms     = (uint32_t)(esp_timer_get_time() / 1000);
        current_status   = SensorStatus::OK;

        SensorReading reading;
        reading.type         = SensorType::DHT22_TEMPERATURE;
        reading.value        = temperature;
        reading.timestamp_ms = last_read_ms;
        reading.status       = SensorStatus::OK;
        return reading;
    }

    // On failure: return last valid reading (or 0.0f if none)
    SensorReading reading;
    reading.type         = SensorType::DHT22_TEMPERATURE;
    reading.value        = last_temperature;
    reading.timestamp_ms = last_read_ms;
    reading.status       = current_status;  // set by readRaw
    return reading;
}

bool DHT22Driver::calibrate()
{
    const int   num_samples = 3;
    float       temps[num_samples];
    float       hums[num_samples];
    int         success_count = 0;

    for (int i = 0; i < num_samples; i++) {
        float t = 0.0f, h = 0.0f;
        if (readRaw(t, h)) {
            temps[success_count] = t;
            hums[success_count]  = h;
            success_count++;
        }
        // Small delay between calibration samples
        esp_rom_delay_us(2000);
    }

    if (success_count < 2) {
        // Need at least 2 good samples for meaningful variance check
        current_status = SensorStatus::ERROR_READ_FAILURE;
        return false;
    }

    // Compute mean temperature and humidity
    float t_mean = 0.0f, h_mean = 0.0f;
    for (int i = 0; i < success_count; i++) {
        t_mean += temps[i];
        h_mean += hums[i];
    }
    t_mean /= (float)success_count;
    h_mean /= (float)success_count;

    // Verify each reading is within 5% of the mean
    const float variance = 0.05f;
    for (int i = 0; i < success_count; i++) {
        if (t_mean > 0.0f) {
            float t_dev = (temps[i] - t_mean) / t_mean;
            if (t_dev < -variance || t_dev > variance) {
                current_status = SensorStatus::ERROR_CALIBRATION;
                return false;
            }
        }
        if (h_mean > 0.0f) {
            float h_dev = (hums[i] - h_mean) / h_mean;
            if (h_dev < -variance || h_dev > variance) {
                current_status = SensorStatus::ERROR_CALIBRATION;
                return false;
            }
        }
    }

    last_temperature = t_mean;
    last_humidity    = h_mean;
    current_status   = SensorStatus::OK;
    return true;
}

SensorStatus DHT22Driver::getStatus() const
{
    return current_status;
}

const char* DHT22Driver::getName() const
{
    return "DHT22";
}

/* ========================================================================== */
/* DHT22 one-wire protocol                                                    */
/* ========================================================================== */

bool DHT22Driver::readRaw(float& temperature, float& humidity)
{
    /* ---- Step 1: Host initiates communication ---- */

    // Configure pin as output, driven low
    gpio_set_direction(data_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(data_pin, 0);

    // Hold low for at least 18ms (datasheet: min 18, typical 20)
    esp_rom_delay_us(20000);

    // Pull high for 20-40us to signal start
    gpio_set_level(data_pin, 1);
    esp_rom_delay_us(40);

    /* ---- Step 2: Switch to input, wait for sensor response ---- */

    gpio_set_direction(data_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(data_pin, GPIO_PULLUP_ONLY);

    uint32_t timeout;

    // Wait for sensor to pull low (response start)
    timeout = esp_timer_get_time();
    while (gpio_get_level(data_pin) == 1) {
        if (esp_timer_get_time() - timeout > 100) {
            current_status = SensorStatus::ERROR_TIMEOUT;
            return false;
        }
    }

    // Sensor holds low for ~80us
    timeout = esp_timer_get_time();
    while (gpio_get_level(data_pin) == 0) {
        if (esp_timer_get_time() - timeout > 100) {
            current_status = SensorStatus::ERROR_TIMEOUT;
            return false;
        }
    }

    // Sensor holds high for ~80us (ready to transmit)
    timeout = esp_timer_get_time();
    while (gpio_get_level(data_pin) == 1) {
        if (esp_timer_get_time() - timeout > 100) {
            current_status = SensorStatus::ERROR_TIMEOUT;
            return false;
        }
    }

    /* ---- Step 3: Read 40 data bits (MSB first) ---- */

    uint8_t data[5] = {0};

    for (int i = 0; i < 40; i++) {
        // Each bit: sensor pulls low for ~50us
        timeout = esp_timer_get_time();
        while (gpio_get_level(data_pin) == 0) {
            if (esp_timer_get_time() - timeout > 80) {
                current_status = SensorStatus::ERROR_READ_FAILURE;
                return false;
            }
        }

        // Sample 30-40us after rising edge
        //   - Bit "0": line stays high ~26-28us
        //   - Bit "1": line stays high ~70us
        esp_rom_delay_us(35);

        int bit = (gpio_get_level(data_pin) == 1) ? 1 : 0;

        data[i / 8] <<= 1;
        data[i / 8]  |= bit;

        // Wait for bit to finish (line goes low)
        timeout = esp_timer_get_time();
        while (gpio_get_level(data_pin) == 1) {
            if (esp_timer_get_time() - timeout > 60) {
                current_status = SensorStatus::ERROR_READ_FAILURE;
                return false;
            }
        }
    }

    /* ---- Step 4: Verify checksum ---- */

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        current_status = SensorStatus::ERROR_READ_FAILURE;
        return false;
    }

    /* ---- Step 5: Parse values ---- */

    // Humidity: bytes 0-1, divide by 10
    humidity = (data[0] * 256.0f + data[1]) / 10.0f;

    // Temperature: bytes 2-3, divide by 10, bit 15 = sign
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    if (raw_temp & 0x8000) {
        temperature = -(float)(raw_temp & 0x7FFF) / 10.0f;
    } else {
        temperature = (float)raw_temp / 10.0f;
    }

    current_status = SensorStatus::OK;
    return true;
}
