---
plan_id: 01-A-SCAFFOLD
wave: 1
depends_on: []
files_modified:
  - platformio.ini
  - CMakeLists.txt
  - config/pins_config.h
  - config/sampling_config.h
  - config/mqtt_config.h
  - config/wifi_config.h
  - hal/isensor.h
  - hal/sensor_reading.h
  - hal/hal.h
autonomous: true
---

# Plan 01-A: Project Scaffold + Config + HAL Interface

## Objective

Crear la estructura base del proyecto PlatformIO con configuración centralizada y la interfaz HAL de abstracción de hardware.

## Requirements

- FWK-01: Proyecto PlatformIO con estructura profesional separando .h/.cpp por módulos
- FWK-02: Capa HAL (Hardware Abstraction Layer) para aislar drivers del hardware específico
- FWK-03: Sistema de configuración centralizada (WiFi, MQTT, pines, umbrales)

## Tasks

### Task 1: Create PlatformIO project structure

Create the directory tree and build configuration for the modular firmware project.

<read_first>
- (No existing files — greenfield project)
</read_first>

<action>
Create directory structure:
- `NodeAlert-Firmware/` root project directory
- `NodeAlert-Firmware/include/` — public headers
- `NodeAlert-Firmware/src/core/` — state machine, system
- `NodeAlert-Firmware/src/hal/` — hardware abstraction interface
- `NodeAlert-Firmware/src/drivers/sensor/` — sensor-specific drivers
- `NodeAlert-Firmware/src/managers/` — coordination layer
- `NodeAlert-Firmware/src/services/` — utilities (serial, timer)
- `NodeAlert-Firmware/src/config/` — centralized configuration headers
- `NodeAlert-Firmware/test/` — test directory

Write `platformio.ini`:
- platform = espressif32
- board = esp32dev
- framework = espidf
- monitor_speed = 115200
- lib_deps = (empty for v1)
- build_flags for all modules

Write `CMakeLists.txt` at root:
- cmake_minimum_required(VERSION 3.16)
- include($ENV{IDF_PATH}/tools/cmake/project.cmake)
- project(NodeAlert-Firmware)

Write `src/CMakeLists.txt`:
- Register all source directories as components
- Register all include paths

Write `.gitignore` for PlatformIO (build/, sdkconfig, .vscode/)
</action>

<acceptance_criteria>
- `platformio.ini` exists with espressif32 platform, esp32dev board, espidf framework
- `CMakeLists.txt` exists with project(NodeAlert-Firmware)
- `src/CMakeLists.txt` registers all component directories
- `pio run` compiles successfully (produces firmware binary)
- All directories listed in <action> exist
</acceptance_criteria>

### Task 2: Create centralized configuration system

Write configuration header files with all constants for the system.

<read_first>
- (No existing files — greenfield project)
</read_first>

<action>
Write `src/config/pins_config.h`:
- GPIO pin definitions for DHT22 (e.g., GPIO_NUM_4), MQ-9 (GPIO_NUM_34, ADC1_CHANNEL_6), KY-026 digital (GPIO_NUM_5), KY-026 analog (GPIO_NUM_35), relay/actuator (GPIO_NUM_2)
- Use #define with descriptive names prefixed with PIN_

Write `src/config/sampling_config.h`:
- SAMPLE_INTERVAL_DHT22_MS = 2000
- SAMPLE_INTERVAL_MQ9_MS = 1000
- SAMPLE_INTERVAL_KY026_MS = 200
- TEMP_THRESHOLD_HIGH_C = 50.0f
- TEMP_THRESHOLD_LOW_C = -10.0f
- HUM_THRESHOLD_HIGH_PCT = 90.0f
- GAS_THRESHOLD_HIGH = 2000
- HYSTERESIS_TEMP = 3.0f
- HYSTERESIS_GAS = 200
- BACKOFF_RETRIES = 3
- BACKOFF_BASE_MS = 1000
- WATCHDOG_TIMEOUT_MS = 10000

Write `src/config/wifi_config.h`:
- Placeholder definitions: WIFI_SSID, WIFI_PASS as empty strings (filled in Phase 3)
- `#ifndef WIFI_SSID` guard for default values

Write `src/config/mqtt_config.h`:
- Placeholder definitions: MQTT_BROKER_URI, MQTT_PORT (1883), MQTT_KEEPALIVE (60), MQTT_TOPIC_PREFIX ("nodealert/")
- All with `#ifndef` guards for compile-time override

Use `#pragma once` on all headers. Group constants by domain with clear section comments.
</action>

<acceptance_criteria>
- `src/config/pins_config.h` defines all 7 GPIO pins with PIN_ prefix
- `src/config/sampling_config.h` defines all 12 constants with correct values
- `src/config/wifi_config.h` has WIFI_SSID, WIFI_PASS with #ifndef guards
- `src/config/mqtt_config.h` has MQTT_BROKER_URI, MQTT_PORT (1883), MQTT_KEEPALIVE (60), MQTT_TOPIC_PREFIX
- All headers use `#pragma once`
- `pio run` still compiles after adding config files
</acceptance_criteria>

### Task 3: Create HAL abstraction layer

Define the sensor interface and reading data structure that all drivers will implement.

<read_first>
- src/config/pins_config.h
</read_first>

<action>
Write `src/hal/sensor_reading.h`:
- Define enum class `SensorType` with values: DHT22_TEMPERATURE, DHT22_HUMIDITY, MQ9_GAS, KY026_FLAME
- Define enum class `SensorStatus` with values: OK, ERROR_TIMEOUT, ERROR_READ_FAILURE, ERROR_NOT_INITIALIZED, ERROR_CALIBRATION
- Define struct `SensorReading` with fields: `SensorType type`, `float value`, `uint32_t timestamp_ms`, `SensorStatus status`
- Constructor with defaults: type, value (0.0f), timestamp (0), status (OK)

Write `src/hal/isensor.h`:
- Define class `ISensor` with pure virtual methods:
  - `virtual SensorReading read() = 0`
  - `virtual bool calibrate() = 0`
  - `virtual SensorStatus getStatus() const = 0`
  - `virtual const char* getName() const = 0`
- Virtual destructor with default implementation

Write `src/hal/hal.h`:
- Convenience include header that bundles sensor_reading.h and isensor.h
- `#pragma once`
</action>

<acceptance_criteria>
- `src/hal/sensor_reading.h` defines SensorType, SensorStatus enums and SensorReading struct
- `src/hal/isensor.h` defines ISensor with 4 pure virtual methods
- `src/hal/hal.h` includes both headers
- `pio run` compiles without errors
</acceptance_criteria>

## Verification Criteria

1. `pio run` exits 0 with no warnings
2. All 3 config headers are readable and linked
3. ISensor interface compiles without implementation errors
4. Directory structure matches the layered design (core/, hal/, drivers/sensor/, managers/, services/, config/)

## Must Haves

- [x] PlatformIO project compiles from CLI
- [x] Config headers accessible from any source file
- [x] HAL interface is ready for driver implementations
