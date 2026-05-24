---
plan_id: 01-B-DHT22
wave: 2
depends_on: ["01-A-SCAFFOLD"]
files_modified:
  - src/drivers/sensor/dht22_driver.h
  - src/drivers/sensor/dht22_driver.cpp
  - src/services/serial_manager.h
  - src/services/serial_manager.cpp
  - src/core/system_core.h
  - src/core/system_core.cpp
  - src/main.cpp
autonomous: true
---

# Plan 01-B: DHT22 Driver + Serial Output

## Objective

Implementar el driver DHT22 (temperatura y humedad) con HAL y mostrar lecturas por serial, verificando el flujo completo sensor → HAL → serial output.

## Requirements

- DRV-01: Driver modular para DHT22 (temperatura y humedad) con HAL
- FWK-04: Máquina de estados del sistema (init → running básico)

## Tasks

### Task 1: Implement DHT22 driver

Create the DHT22 sensor driver implementing the ISensor interface with timing protocol for DHT22 communication.

<read_first>
- src/hal/isensor.h
- src/hal/sensor_reading.h
- src/config/pins_config.h
- src/config/sampling_config.h
</read_first>

<action>
Write `src/drivers/sensor/dht22_driver.h`:
- Class `DHT22Driver` publicly inheriting `ISensor`
- Constructor takes `gpio_num_t pin`
- Virtual method overrides: `read()`, `calibrate()`, `getStatus()`, `getName()`
- Private: `gpio_num_t data_pin`, `SensorStatus current_status`, `uint32_t last_read_ms`
- Private method: `bool readRaw(float& temperature, float& humidity)` for the one-wire protocol

Write `src/drivers/sensor/dht22_driver.cpp`:
- Implement DHT22 one-wire protocol:
  - Pull low for 18ms, pull high for 40us, wait for response
  - Read 40 bits (16 humidity, 16 temperature, 8 checksum)
  - Parse: humidity = bits[0:15] / 10.0, temperature = bits[16:31] / 10.0
  - Check checksum (bits[32:39] == low 8 bits of humidity+temp bytes)
- `read()`:
  - Call `readRaw()`, on success store values in SensorReading
  - On failure: set ERROR_READ_FAILURE, return previous value if available, else 0.0f
  - Update current_status, last_read_ms
- `calibrate()`: read 3 samples, verify consistent readings, return true if within 5% variance
- `getName()`: return "DHT22"
- Add `#include "driver/gpio.h"`, `#include "esp_timer.h"` for GPIO and timing

Use `gpio_set_level`, `gpio_get_level`, `esp_timer_get_time()` for microsecond timing.
Use `ets_delay_us()` for microsecond delays in the protocol.
</action>

<acceptance_criteria>
- `DHT22Driver` class implements all 4 ISensor methods
- DHT22 one-wire protocol implemented correctly (18ms low, 40us high, 40-bit read)
- Checksum validation in readRaw
- `read()` returns SensorReading with type DHT22_TEMPERATURE and DHT22_HUMIDITY
- Error handling for timeout and checksum mismatch returns ERROR_READ_FAILURE
- `pio run` compiles without errors
</acceptance_criteria>

### Task 2: Create serial output service

Implement a serial manager for formatted output of sensor readings.

<read_first>
- src/hal/sensor_reading.h
- src/hal/isensor.h
</read_first>

<action>
Write `src/services/serial_manager.h`:
- Class `SerialManager` with static methods
- `static void init(int baud_rate)` — configure UART
- `static void printReading(const SensorReading& reading)` — formatted sensor output
- `static void printState(const char* state_name)` — state transition notifications
- `static void printError(const char* source, const char* message)` — error messages

Write `src/services/serial_manager.cpp`:
- `init(115200)` — use `stdio` via ESP-IDF console
- `printReading()` format: `[timestamp] SENSOR:{type} VALUE:{value} STATUS:{status}`
  - Example: `[1000] SENSOR:DHT22_TEMPERATURE VALUE:25.3 STATUS:OK`
- `printState()` format: `[timestamp] STATE:{state_name}`
- `printError()` format: `[timestamp] ERROR:{source} MSG:{message}`
</action>

<acceptance_criteria>
- `SerialManager::init(115200)` configures UART output
- `printReading()` outputs exactly `[timestamp] SENSOR:{type} VALUE:{value} STATUS:{status}\n`
- `printState()` outputs exactly `[timestamp] STATE:{state_name}\n`
- `pio run` compiles
</acceptance_criteria>

### Task 3: Create main application with init state and DHT22 loop

Wire DHT22 driver into main.cpp with basic init → running state machine.

<read_first>
- src/drivers/sensor/dht22_driver.h
- src/services/serial_manager.h
- src/config/sampling_config.h
- src/config/pins_config.h
</read_first>

<action>
Write `src/core/system_core.h`:
- Enum class `SystemState`: INIT, STANDBY, RUNNING, ALERT, ERROR, RECOVERY
- Function `const char* stateToString(SystemState state)` for display

Write `src/core/system_core.cpp`:
- Implement `stateToString()` mapping each enum to string

Write `src/main.cpp`:
- `extern "C" void app_main(void)` entry point
- Initialize NVS flash (`nvs_flash_init()`)
- Initialize SerialManager with 115200 baud
- Create DHT22Driver on PIN_DHT22 (GPIO_NUM_4)
- Print INIT state
- Transition to RUNNING state via `stateToString(SystemState::RUNNING)`
- Loop every SAMPLE_INTERVAL_DHT22_MS:
  - Call `dht22.read()`
  - Print reading via `SerialManager::printReading()`
  - Print current state
- Basic error handling on read failure
</action>

<acceptance_criteria>
- `src/core/system_core.h` defines SystemState enum with INIT, STANDBY, RUNNING, ALERT, ERROR, RECOVERY
- `stateToString()` returns correct strings for all states
- `src/main.cpp` calls `nvs_flash_init()`, creates DHT22Driver, enters RUNNING loop
- Loop reads sensor every SAMPLE_INTERVAL_DHT22_MS (2000ms)
- `pio run` compiles without errors
- Flashing to ESP32 outputs formatted readings over serial
</acceptance_criteria>

## Verification Criteria

1. `pio run` exits 0
2. DHT22 driver reads temperature and humidity correctly on real hardware
3. Serial output format matches: `[timestamp] SENSOR:DHT22_TEMPERATURE VALUE:25.3 STATUS:OK`
4. State transitions printed on serial
5. Error handling works when DHT22 is disconnected (prints ERROR status)

## Must Haves

- [x] DHT22 driver compiles and links
- [x] Serial output shows formatted readings
- [x] Basic init → running state transition
