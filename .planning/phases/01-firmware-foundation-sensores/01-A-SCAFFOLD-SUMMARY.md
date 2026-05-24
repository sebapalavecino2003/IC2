---
phase: 01-firmware-foundation-sensores
plan: 01-A-SCAFFOLD
subsystem: firmware
tags: [platformio, esp32, espidf, hal, cmake]

# Dependency graph
requires: []
provides:
  - PlatformIO project structure with modular directories
  - Centralized configuration system (pins, sampling, WiFi, MQTT)
  - HAL abstraction layer interface (ISensor, SensorReading)
  - Build-tested firmware binary for ESP32
affects:
  - 01-B-DHT22 (driver implementation)
  - 01-C-MQ9-KY026 (driver implementation)
  - 01-D-RTOS-INTEGRATION (manager/services layer)

# Tech tracking
tech-stack:
  added:
    - PlatformIO 6.1.19 with ESP-IDF 6.0.1
    - ESP32 toolchain (xtensa-esp-elf-gcc 15.2.0)
  patterns:
    - Modular directory structure with core/hal/drivers/managers/services/config separation
    - Header-only configuration system with #pragma once
    - Pure virtual interface (ISensor) for hardware abstraction

key-files:
  created:
    - NodeAlert-Firmware/platformio.ini
    - NodeAlert-Firmware/CMakeLists.txt
    - NodeAlert-Firmware/src/core/main.cpp
    - NodeAlert-Firmware/src/config/pins_config.h
    - NodeAlert-Firmware/src/config/sampling_config.h
    - NodeAlert-Firmware/src/config/wifi_config.h
    - NodeAlert-Firmware/src/config/mqtt_config.h
    - NodeAlert-Firmware/src/hal/sensor_reading.h
    - NodeAlert-Firmware/src/hal/isensor.h
    - NodeAlert-Firmware/src/hal/hal.h
    - NodeAlert-Firmware/.gitignore
  modified: []

key-decisions:
  - "Removed hand-written src/CMakeLists.txt — PlatformIO auto-generates ESP-IDF component registration with idf_component_register"
  - "Include paths managed via build_flags in platformio.ini instead of CMake target_include_directories"
  - "Added minimal main.cpp entry point (ESP-IDF requires at least one source file to link)"

patterns-established:
  - "Pure virtual interface for sensor drivers: ISensor base class with read/calibrate/getStatus/getName"
  - "Configuration via #define constants grouped by domain with #ifndef guards for compile-time overrides"
  - "Convenience header (hal.h) bundling full module interfaces"

requirements-completed:
  - FWK-01
  - FWK-02
  - FWK-03

# Metrics
duration: 10min
completed: 2026-05-24
---

# Phase 01 Plan A: Project Scaffold + Config + HAL Interface — Summary

**PlatformIO project scaffold for NodeAlert IoT: modular directory structure, centralized configuration headers, and hardware abstraction layer interface, all verified with a successful `pio run` cross-compilation**

## Performance

- **Duration:** 10 min
- **Started:** 2026-05-24T23:47:25Z
- **Completed:** 2026-05-24T23:58:05Z
- **Tasks:** 3 (all type=auto)
- **Files modified:** 11 source files + build config + gitignore

## Accomplishments

- PlatformIO project structure with 7 source directories (core/, hal/, drivers/sensor/, managers/, services/, config/) plus include/ and test/
- Build-tested with `pio run` — both `esp32dev` and `test` environments compile successfully producing firmware binaries
- Centralized configuration system with 4 headers covering GPIO pins, sampling intervals/thresholds, WiFi placeholders, and MQTT broker settings
- Complete HAL abstraction: `ISensor` pure virtual interface, `SensorType`/`SensorStatus` enums, and `SensorReading` struct with default constructor
- Minimal ESP-IDF `app_main` entry point for future integration phases

## Task Commits

Each task was committed atomically:

1. **Task 1: Create PlatformIO project structure** — `9148ecf` (feat)
2. **Task 2: Create centralized configuration system** — `3c1b7a0` (feat)
3. **Task 3: Create HAL abstraction layer** — `fcdaffc` (feat)
4. **Plan metadata: post-task cleanup** — `ca7f304` (chore)

## Files Created/Modified

- `NodeAlert-Firmware/platformio.ini` — PlatformIO build config (espressif32, esp32dev, espidf)
- `NodeAlert-Firmware/CMakeLists.txt` — ESP-IDF project root CMake
- `NodeAlert-Firmware/src/CMakeLists.txt` — Auto-generated ESP-IDF component registration
- `NodeAlert-Firmware/.gitignore` — PlatformIO/ESP-IDF/IDE artifacts
- `NodeAlert-Firmware/src/core/main.cpp` — Minimal ESP-IDF entry point
- `NodeAlert-Firmware/src/config/pins_config.h` — GPIO pin definitions (DHT22, MQ-9, KY-026, relay)
- `NodeAlert-Firmware/src/config/sampling_config.h` — Sampling intervals, thresholds, hysteresis
- `NodeAlert-Firmware/src/config/wifi_config.h` — WiFi credentials (placeholder with #ifndef guards)
- `NodeAlert-Firmware/src/config/mqtt_config.h` — MQTT broker/topic config (placeholder with #ifndef guards)
- `NodeAlert-Firmware/src/hal/sensor_reading.h` — SensorType, SensorStatus enums, SensorReading struct
- `NodeAlert-Firmware/src/hal/isensor.h` — ISensor pure virtual interface (4 methods + virtual dtor)
- `NodeAlert-Firmware/src/hal/hal.h` — Convenience header bundling HAL components

## Decisions Made

- **Build system adaptation:** The plan specified a manual `src/CMakeLists.txt` with `target_include_directories`, which is incompatible with ESP-IDF's component-based CMake. PlatformIO auto-generates a `src/CMakeLists.txt` using `idf_component_register(SRCS ${app_sources})`. Include paths are managed via `build_flags` in `platformio.ini` instead.
- **Minimal entry point:** Added `main.cpp` with `app_main()` loop — ESP-IDF requires at least one source file to produce a linkable binary. This entry point will be replaced by the state machine in later phases.

## Deviations from Plan

### Rule 3 - Build Configuration Adaptation

**1. [Rule 3 - Blocking] src/CMakeLists.txt incompatible with ESP-IDF build system**
- **Found during:** Task 1 (PlatformIO project structure)
- **Issue:** The plan specified `src/CMakeLists.txt` using `target_include_directories`, which is not scriptable in ESP-IDF's component context. Build failed with CMake error.
- **Fix:** Removed the hand-written `src/CMakeLists.txt`. PlatformIO auto-generates the correct ESP-IDF component registration via `idf_component_register(SRCS ${app_sources})`. Include paths are configured through `build_flags` in `platformio.ini`.
- **Files modified:** Removed `src/CMakeLists.txt` (hand-written), PlatformIO auto-generated equivalent; `platformio.ini` (build_flags)
- **Verification:** `pio run` succeeded for both environments
- **Committed in:** `9148ecf` (Task 1), `ca7f304` (post-task commit)

**2. [Rule 3 - Blocking] Missing ESP-IDF entry point (main.cpp)**
- **Found during:** Task 1 (PlatformIO project structure)
- **Issue:** ESP-IDF requires at least one source file with `app_main()` to link a firmware binary. Empty project would not compile.
- **Fix:** Added minimal `main.cpp` in `src/core/` with `app_main()` that initializes and idles.
- **Files modified:** `NodeAlert-Firmware/src/core/main.cpp`
- **Verification:** `pio run` succeeded, firmware binary produced
- **Committed in:** `9148ecf` (Task 1)

---

**Total deviations:** 2 auto-fixed (2 blocking build issues)
**Impact on plan:** Both fixes necessary for build system compatibility. No scope creep — entry point will evolve naturally in later phases.

## Issues Encountered

- PlatformIO ESP-IDF build requires exact component structure. Initial `src/CMakeLists.txt` using `target_include_directories` failed — corrected by relying on PlatformIO's auto-generated component registration.
- First PlatformIO run required downloading ESP32 toolchain (~200MB) and ESP-IDF Python dependencies — added ~3 minutes to initial build time.
- PlatformIO 6.1.19 deprecated `test_build_project_src` in favor of `test_build_src` — updated in platformio.ini.

## Stub Tracking

- `src/config/wifi_config.h`: WIFI_SSID and WIFI_PASS are empty strings — intentional, will be populated in Phase 3 (connectivity).
- `src/config/mqtt_config.h`: MQTT_BROKER_URI is empty string — intentional, will be populated in Phase 3.
- `src/core/main.cpp`: Contains only a placeholder idle loop — intentional, will be replaced by state machine in Phase 1-D.

## Threat Flags

None — scaffold/config/HAL files contain no network endpoints, auth paths, or mutable data at trust boundaries.

## Next Phase Readiness

- **Ready for 01-B-DHT22:** HAL interface defined, config system available, build verified. DHT22 driver can implement `ISensor` and use `pins_config.h` / `sampling_config.h`.
- **Ready for 01-C-MQ9-KY026:** Same foundation available for gas and flame sensor drivers.
- **Ready for 01-D-RTOS-INTEGRATION:** PlatformIO project compiles, main entry point exists for RTOS task integration.
- All three scaffold requirements (FWK-01, FWK-02, FWK-03) are fulfilled.

## Self-Check: PASSED

- ✅ All 12 created files verified on disk
- ✅ All 5 commits confirmed in git log
- ✅ `pio run` succeeds for both `esp32dev` and `test` environments
- ✅ Directory structure matches layered design (7 source directories)

---

*Phase: 01-firmware-foundation-sensores*
*Completed: 2026-05-24*
