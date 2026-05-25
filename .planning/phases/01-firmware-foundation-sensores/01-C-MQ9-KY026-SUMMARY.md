---
phase: 01-firmware-foundation-sensores
plan: 01-C-MQ9-KY026
subsystem: firmware
tags: [mq9, ky026, gas-sensor, flame-sensor, adc, calibration, interrupt, isr, esp32, espidf]

# Dependency graph
requires:
  - phase: 01-A-SCAFFOLD
    provides: HAL interface (ISensor, SensorReading), config system, build-tested PlatformIO project
  - phase: 01-B-DHT22
    provides: ISensor implementation pattern, SerialManager, SystemState enum
provides:
  - MQ-9 gas sensor driver (CO + flammable gases) with 60s preheat and ADC ratio estimation
  - KY-026 flame sensor driver with hardware interrupt (rising edge) and analog intensity reading
  - Shared ADC1 module for ESP-IDF oneshot handle-based API
  - SensorCalibration reusable module (moving average, median, stability check)
affects:
  - 01-D-RTOS-INTEGRATION (drivers ready for FreeRTOS task orchestration)

# Tech tracking
tech-stack:
  added:
    - ESP-IDF adc_oneshot API (handle-based ADC for ESP-IDF 6.x)
    - C++ std::vector + std::sort for calibration ring buffer
    - gpio_install_isr_service / gpio_isr_handler_add for HW interrupt handling
  patterns:
    - Shared ADC1 handle via nodealert_adc1_init() singleton for multi-driver ADC sharing
    - Static IRAM_ATTR ISR handler dispatching to instance via void* arg
    - Ring-buffer-based SensorCalibration with vector pre-allocation

key-files:
  created:
    - NodeAlert-Firmware/src/drivers/sensor/mq9_driver.h
    - NodeAlert-Firmware/src/drivers/sensor/mq9_driver.cpp
    - NodeAlert-Firmware/src/drivers/sensor/ky026_driver.h
    - NodeAlert-Firmware/src/drivers/sensor/ky026_driver.cpp
    - NodeAlert-Firmware/src/drivers/sensor/calibration.h
    - NodeAlert-Firmware/src/drivers/sensor/calibration.cpp
    - NodeAlert-Firmware/src/hal/adc_shared.h
    - NodeAlert-Firmware/src/hal/adc_shared.cpp
  modified:
    - NodeAlert-Firmware/src/config/pins_config.h
    - NodeAlert-Firmware/src/config/sampling_config.h
    - NodeAlert-Firmware/src/hal/hal.h

key-decisions:
  - "Replaced legacy adc1_get_raw() with modern ESP-IDF adc_oneshot handle API (legacy ADC API removed in ESP-IDF 6.0.1)"
  - "Created shared ADC1 module (adc_shared.h/cpp) as a singleton so MQ-9 and KY-026 can share the same ADC unit handle"
  - "ADC_CHANNEL_7 used for KY-026 analog input (GPIO 35 → ADC1 CH7 on ESP32)"
  - "KY-026 ISR handler uses xTaskGetTickCountFromISR() instead of esp_timer_get_time() (ISR-safe)"
  - "SensorCalibration uses std::vector<float> with pre-allocation for deterministic memory usage"

patterns-established:
  - "Sensor drivers implement ISensor and use nodealert_adc1_init() for ADC access"
  - "ISR handler as static IRAM_ATTR member function dispatching via this pointer"
  - "Ring buffer pattern for sensor data filtering (pre-allocated std::vector)"

requirements-completed:
  - DRV-02
  - DRV-03
  - DRV-04

# Metrics
duration: 10min
completed: 2026-05-25
---

# Phase 1 Plan C: MQ-9 + KY-026 Drivers + Calibration — Summary

**MQ-9 gas sensor with 60s preheat and ADC ratio estimation, KY-026 flame sensor with HW rising-edge interrupt and analog intensity reading, and reusable SensorCalibration module for moving average/median/stability filtering — all verified with `pio run` cross-compilation**

## Performance

- **Duration:** 10 min
- **Started:** 2026-05-25T00:10:16Z
- **Completed:** 2026-05-25T00:20:30Z
- **Tasks:** 3 (all type=auto)
- **Files modified:** 10 (8 created, 2 modified)

## Accomplishments

- **MQ-9 gas sensor driver (DRV-02):** Full ISensor implementation with ADC reading via ESP-IDF adc_oneshot API, 60-second preheat timing with ERROR_CALIBRATION guard, calibrate() reading 10 ADC samples with 10% variance check, gas concentration ratio estimation as percentage change from clean-air baseline voltage
- **KY-026 flame sensor driver (DRV-03):** ISensor implementation with GPIO rising-edge interrupt (GPIO_INTR_POSEDGE), static IRAM_ATTR ISR handler with volatile flame_detected flag, digital+analog reading paths, calibrate() with 5-sample ambient baseline, destructor cleanup of ISR handler
- **Shared ADC1 module:** Created `adc_shared.h/cpp` singleton to share the ADC_UNIT_1 handle between MQ-9 and KY-026 drivers (ESP-IDF 6.x requires handle-based API and only one handle per ADC unit)
- **SensorCalibration module (DRV-04):** Reusable ring-buffer-based class with moving average, median filtering (std::sort copy), and variance-based stability detection, with pre-allocated std::vector storage
- **Configuration updates:** Added PIN_KY026_ADC_CHANNEL (ADC_CHANNEL_7) to pins_config.h, CALIBRATION_SAMPLES (10) and CALIBRATION_VARIANCE_THRESHOLD (5.0f) to sampling_config.h

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement MQ-9 gas sensor driver** — `4ed1989` (feat)
2. **Task 2: Implement KY-026 flame sensor driver with HW interrupt** — `2005abf` (feat)
3. **Task 3: Create sensor calibration and filtering module** — `d333296` (feat)

## Files Created/Modified

- `NodeAlert-Firmware/src/drivers/sensor/mq9_driver.h` — MQ9Driver header (ISensor, ADC channel, power pin)
- `NodeAlert-Firmware/src/drivers/sensor/mq9_driver.cpp` — 60s preheat, ADC read, gas ratio calibration
- `NodeAlert-Firmware/src/drivers/sensor/ky026_driver.h` — KY026Driver header (digital/analog, ISR, volatile flag)
- `NodeAlert-Firmware/src/drivers/sensor/ky026_driver.cpp` — GPIO interrupt, ADC intensity, destructor cleanup
- `NodeAlert-Firmware/src/drivers/sensor/calibration.h` — SensorCalibration header (moving avg, median, stability)
- `NodeAlert-Firmware/src/drivers/sensor/calibration.cpp` — Ring buffer with std::vector, std::sort median
- `NodeAlert-Firmware/src/hal/adc_shared.h` — Shared ADC1 handle init function declaration
- `NodeAlert-Firmware/src/hal/adc_shared.cpp` — Singleton ADC1 handle (init-once, reuse pattern)
- `NodeAlert-Firmware/src/hal/hal.h` — Added `#include "hal/adc_shared.h"`
- `NodeAlert-Firmware/src/config/pins_config.h` — Added PIN_KY026_ADC_CHANNEL (ADC_CHANNEL_7)
- `NodeAlert-Firmware/src/config/sampling_config.h` — Added CALIBRATION_SAMPLES, CALIBRATION_VARIANCE_THRESHOLD

## Decisions Made

- **ESP-IDF adc_oneshot API:** ESP-IDF 6.0.1 completely removed the legacy `adc1_get_raw()` / `adc1_config_width()` / `adc1_config_channel_atten()` API. Replaced with the modern handle-based `adc_oneshot` API throughout both drivers (deviation from plan's `driver/adc.h` includes).
- **Shared ADC handle:** Since the new API only allows one handle per ADC unit, created `adc_shared.h/cpp` as a singleton pattern in the HAL layer — first driver to init creates the handle, subsequent calls reuse it.
- **ADC channel constants:** The plan specified `ADC1_CHANNEL_6` (legacy). In ESP-IDF 6.x, ADC channels are `ADC_CHANNEL_0` through `ADC_CHANNEL_7` (without the "1_"). Updated pins_config.h to use `ADC_CHANNEL_7` for KY-026 analog.
- **ISR-safe timing:** The KY-026 ISR handler uses `xTaskGetTickCountFromISR()` instead of `esp_timer_get_time()` (the latter is not ISR-safe).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Legacy ADC API removed in ESP-IDF 6.0.1**

- **Found during:** Task 1 (MQ-9 driver)
- **Issue:** The plan specified `#include "driver/adc.h"` and `adc1_get_raw()` / `adc1_config_width()` / `adc1_config_channel_atten()` — this legacy ADC API was removed in ESP-IDF 6.0.1. The modern API uses `adc_oneshot_new_unit()`, `adc_oneshot_config_channel()`, and `adc_oneshot_read()`.
- **Fix:** Created `src/hal/adc_shared.h` and `src/hal/adc_shared.cpp` providing a shared ADC1 unit handle singleton. Both MQ-9 and KY-026 drivers use `nodealert_adc1_init()` to obtain the handle and `adc_oneshot_*` API for reading. Only one handle is created per ADC unit.
- **Files modified:** Created `src/hal/adc_shared.h`, `src/hal/adc_shared.cpp`; modified `src/hal/hal.h`; rewrote `mq9_driver.h` and `mq9_driver.cpp` to use `adc_channel_t` and `adc_oneshot` API
- **Verification:** `pio run` succeeds for both environments
- **Committed in:** `4ed1989` (Task 1)

**2. [Rule 3 - Blocking] ADC channel type mismatch**

- **Found during:** Task 1 (MQ-9 driver)
- **Issue:** `adc_channel_t` is the modern type alias for ADC channels (`ADC_CHANNEL_0`–`ADC_CHANNEL_7`) instead of the legacy `adc1_channel_t`. The type must be declared with an include of `hal/adc_types.h`.
- **Fix:** Added `#include "hal/adc_types.h"` to `mq9_driver.h` and `ky026_driver.h` for the `adc_channel_t` type
- **Files modified:** `mq9_driver.h`, `ky026_driver.h`
- **Verification:** `pio run` succeeds
- **Committed in:** `4ed1989` (Task 1), `2005abf` (Task 2)

---

**Total deviations:** 2 auto-fixed (2 blocking build issues)
**Impact on plan:** Both fixes necessary for successful compilation on ESP-IDF 6.0.1 toolchain. The shared ADC module is a clean architectural pattern that future drivers can reuse. No scope creep.

## Issues Encountered

- **Legacy ADC API completely removed:** The `esp_adc/adc_legacy.h` header, which was expected to provide backward compatibility for the `adc1_get_raw()` API, does not exist in the PlatformIO ESP-IDF 6.0.1 package (framework-espidf 4.60001.0). The modern `adc_oneshot` API is the only available ADC interface, requiring a handle-based approach with singleton sharing across drivers.
- **KY-026 ADC channel mapping:** GPIO 35 on ESP32 maps to ADC1 channel 7 (not channel 6 like MQ-9's GPIO 34). Added `PIN_KY026_ADC_CHANNEL` constant to `pins_config.h` for clarity.

## Stub Tracking

- `mq9_driver.cpp`: `calibrate()` has a hard-coded 60-second `vTaskDelay` — intentional per plan, acceptable because this runs in a FreeRTOS task context.
- `ky026_driver.cpp`: `calibrate()` stores baseline but does not use it for threshold comparison — intentional, will be used when the sensor manager applies thresholds in 01-D-RTOS-INTEGRATION.

## Threat Flags

None — all files operate at the sensor/firmware layer with no network endpoints, auth paths, or mutable data at trust boundaries. GPIO and ADC access is hardware-mediated.

## Next Phase Readiness

- **Ready for 01-D-RTOS-INTEGRATION:** All three sensor drivers (DHT22, MQ-9, KY-026) implement ISensor, have passed `pio run` compilation, and are ready for FreeRTOS task orchestration with priority-based scheduling (KY-026 highest, MQ-9 high, DHT22 normal).
- Requirements DRV-02, DRV-03, and DRV-04 are fulfilled.
- The shared ADC1 handle pattern is established for future sensor additions.

## Self-Check: PASSED

- ✅ All 8 created files verified on disk
- ✅ All 3 commits confirmed in git log
- ✅ `pio run` succeeds for both `esp32dev` and `test` environments
- ✅ `MQ9Driver` implements all 4 ISensor methods with ADC oneshot API
- ✅ `calibrate()` sets baseline from 10 ADC samples with 10% variance check
- ✅ `read()` returns `SensorReading` with type `MQ9_GAS`
- ✅ `KY026Driver` implements all 4 ISensor methods with HW interrupt
- ✅ GPIO rising-edge interrupt configured with static `IRAM_ATTR` ISR handler
- ✅ `read()` returns 1.0f when flame detected via interrupt
- ✅ `SensorCalibration` class works with configurable window size
- ✅ `getMovingAverage()` and `getMedian()` compute correct values
- ✅ `isStable(threshold)` returns true when readings within variance

---
*Phase: 01-firmware-foundation-sensores*
*Completed: 2026-05-25*
