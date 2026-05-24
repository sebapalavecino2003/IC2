---
plan_id: 01-C-MQ9-KY026
wave: 2
depends_on: ["01-A-SCAFFOLD"]
files_modified:
  - src/drivers/sensor/mq9_driver.h
  - src/drivers/sensor/mq9_driver.cpp
  - src/drivers/sensor/ky026_driver.h
  - src/drivers/sensor/ky026_driver.cpp
  - src/drivers/sensor/calibration.h
  - src/drivers/sensor/calibration.cpp
  - src/config/pins_config.h
  - src/config/sampling_config.h
autonomous: true
---

# Plan 01-C: MQ-9 + KY-026 Drivers + Calibration

## Objective

Implementar los drivers MQ-9 (gas) y KY-026 (llama con interrupción HW), más el módulo de calibración y filtrado.

## Requirements

- DRV-02: Driver modular para MQ-9 (gases inflamables y CO) con HAL
- DRV-03: Driver modular para KY-026 (detección de llama) con HAL
- DRV-04: Módulo de calibración y filtrado de lecturas de sensores

## Tasks

### Task 1: Implement MQ-9 gas sensor driver

Create the MQ-9 driver with ADC reading, preheating detection and gas concentration estimation.

<read_first>
- src/hal/isensor.h
- src/hal/sensor_reading.h
- src/config/pins_config.h
- src/config/sampling_config.h
</read_first>

<action>
Write `src/drivers/sensor/mq9_driver.h`:
- Class `MQ9Driver` publicly inheriting `ISensor`
- Constructor takes `adc1_channel_t adc_channel`, `gpio_num_t power_pin`
- Virtual overrides: `read()`, `calibrate()`, `getStatus()`, `getName()`
- Private: `adc1_channel_t channel`, `gpio_num_t pwr_pin`, `SensorStatus current_status`, `bool preheated`, `uint32_t preheat_start_ms`, `float baseline_voltage`, `float last_reading`

Write `src/drivers/sensor/mq9_driver.cpp`:
- `calibrate()`:
  - Power on sensor via pwr_pin (if separate)
  - Wait 60s preheat (set `preheated = false`, record `preheat_start_ms`)
  - After preheat: read 10 ADC samples, compute average as `baseline_voltage`
  - Return true if readings stabilize within 10% variance
- `read()`:
  - If not preheated and `preheat_start_ms + 60000 > now`: return with status ERROR_CALIBRATION
  - Read ADC via `adc1_get_raw()`, convert to voltage
  - Estimate gas concentration ratio: `(voltage - baseline) / baseline * 100`
  - Return SensorReading with type MQ9_GAS
  - On ADC error: return ERROR_READ_FAILURE
- `getName()`: return "MQ-9"
- Configure ADC: `adc1_config_width(ADC_WIDTH_BIT_12)`, `adc1_config_channel_atten(channel, ADC_ATTEN_DB_12)`
- Use `driver/adc.h`, `driver/gpio.h` includes
</action>

<acceptance_criteria>
- `MQ9Driver` implements all 4 ISensor methods
- ADC configured with 12-bit width and 12dB attenuation
- Calibrate() sets baseline_voltage from 10 ADC samples
- `read()` returns SensorReading with type MQ9_GAS
- Sensor reads gas level via ADC channel
- `pio run` compiles without errors
</acceptance_criteria>

### Task 2: Implement KY-026 flame sensor driver with HW interrupt

Create the KY-026 driver with hardware interrupt support for immediate flame detection.

<read_first>
- src/hal/isensor.h
- src/hal/sensor_reading.h
- src/config/pins_config.h
- src/config/sampling_config.h
</read_first>

<action>
Write `src/drivers/sensor/ky026_driver.h`:
- Class `KY026Driver` publicly inheriting `ISensor`
- Constructor takes `gpio_num_t digital_pin`, `adc1_channel_t analog_channel`
- Virtual overrides: `read()`, `calibrate()`, `getStatus()`, `getName()`
- Static method: `static void IRAM_ATTR interruptHandler(void* arg)` — ISR handler
- Private: `gpio_num_t dig_pin`, `adc1_channel_t ana_channel`, `SensorStatus current_status`, `volatile bool flame_detected`, `uint32_t last_interrupt_ms`
- Private: `bool readDigital()`, `int readAnalog()`

Write `src/drivers/sensor/ky026_driver.cpp`:
- Constructor:
  - Configure digital_pin as input with pull-down
  - Set interrupt type to GPIO_INTR_POSEDGE (rising edge = flame detected)
  - Install ISR service: `gpio_install_isr_service(0)`
  - Add ISR handler: `gpio_isr_handler_add(digital_pin, interruptHandler, this)`
- `interruptHandler()`:
  - Set `flame_detected = true`, record `last_interrupt_ms = xTaskGetTickCountFromISR()`
  - Only `volatile` writes allowed in ISR
- `read()`:
  - If `flame_detected`: return SensorReading with value 1.0f
  - Else: read digital pin, if low (no flame) return 0.0f
  - Read analog channel for flame intensity (0-4095)
  - Reset `flame_detected = false` after reading
- `calibrate()`:
  - Read analog 5 times in known no-flame condition
  - Compute baseline ambient reading
  - Return true
- `getName()`: return "KY-026"
- Use `driver/gpio.h`, `driver/adc.h` includes
- Add `#include "esp_intr_alloc.h"` for ISR
</action>

<acceptance_criteria>
- `KY026Driver` implements all 4 ISensor methods
- GPIO interrupt configured on rising edge (positive edge = flame)
- ISR handler sets `flame_detected` flag
- `read()` returns 1.0f when flame detected via interrupt, 0.0f otherwise
- Analog channel reads flame intensity (0-4095)
- `pio run` compiles without errors
</acceptance_criteria>

### Task 3: Create sensor calibration and filtering module

Implement a reusable calibration and signal processing module for sensor reading stabilization.

<read_first>
- src/hal/sensor_reading.h
</read_first>

<action>
Write `src/drivers/sensor/calibration.h`:
- Class `SensorCalibration`:
  - Constructor with `size_t window_size` (for moving average)
  - `void addSample(float value)` — add reading to buffer
  - `float getMovingAverage()` — compute average of last N samples
  - `float getMedian()` — median of current buffer
  - `bool isStable(float variance_threshold)` — check if readings within variance
  - `void reset()` — clear buffer
  - Private: `std::vector<float> buffer`, `size_t max_samples`, `size_t current_index`

Write `src/drivers/sensor/calibration.cpp`:
- Implement moving average: sum of buffer / count
- Implement median: sort buffer copy, return middle value
- Implement stability check: variance across buffer < threshold
- `reset()`: clear buffer, reset index

Update `src/config/sampling_config.h`:
- Add `CALIBRATION_SAMPLES = 10`
- Add `CALIBRATION_VARIANCE_THRESHOLD = 5.0f`
</action>

<acceptance_criteria>
- `SensorCalibration` class works with configurable window size
- `getMovingAverage()` returns correct average of added samples
- `getMedian()` returns correct median value
- `isStable(threshold)` returns true when readings within variance
- `pio run` compiles without errors
</acceptance_criteria>

## Verification Criteria

1. `pio run` exits 0
2. MQ-9 driver ADC readings return non-zero values on real hardware
3. KY-026 detects flame via interrupt (digital pin goes high)
4. Calibration module correctly computes moving average and median

## Must Haves

- [x] MQ-9 driver with ADC and preheating
- [x] KY-026 driver with HW interrupt detection
- [x] Calibration module for all sensors
