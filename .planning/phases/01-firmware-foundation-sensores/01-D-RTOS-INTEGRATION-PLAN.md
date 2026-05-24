---
plan_id: 01-D-RTOS-INTEGRATION
wave: 3
depends_on: ["01-B-DHT22", "01-C-MQ9-KY026"]
files_modified:
  - src/core/state_machine.h
  - src/core/state_machine.cpp
  - src/core/error_handler.h
  - src/core/error_handler.cpp
  - src/managers/sensor_manager.h
  - src/managers/sensor_manager.cpp
  - src/managers/task_manager.h
  - src/managers/task_manager.cpp
  - src/main.cpp
autonomous: true
---

# Plan 01-D: FreeRTOS Tasks + State Machine + Integration

## Objective

Integrar todos los componentes con tareas FreeRTOS, colas, mutex, la máquina de estados completa y manejo de errores con auto-recovery.

## Requirements

- RTOS-01: Tareas FreeRTOS desacopladas por función
- RTOS-02: Colas para comunicación entre tareas
- RTOS-03: Mutex para acceso seguro a recursos compartidos
- RTOS-04: Prioridades de tareas bien definidas
- RTOS-05: Interrupciones de hardware para eventos críticos
- FWK-04: Máquina de estados del sistema
- FWK-05: Gestión de errores y recuperación

## Tasks

### Task 1: Create complete state machine with all transitions

Implement the full 6-state machine: init → standby → running → alert → error → recovery.

<read_first>
- src/core/system_core.h
- src/config/sampling_config.h
</read_first>

<action>
Write `src/core/state_machine.h`:
- Class `StateMachine`:
  - Constructor with no args
  - `SystemState getCurrentState()`
  - `bool transitionTo(SystemState new_state)` — validate and execute transition
  - `bool isTransitionValid(SystemState from, SystemState to)` — check allowed transitions
  - `const char* getStateName()`
  - `void setAlertThresholds(float temp_high, float gas_high)` — configure alert triggers
  - Private: `SystemState current_state`, `SystemState previous_state`, `uint32_t state_enter_time`
  - Private: `float alert_temp_high`, `float alert_gas_high`

Write `src/core/state_machine.cpp`:
- Valid transitions matrix:
  - INIT → STANDBY (on successful init)
  - STANDBY → RUNNING (on all sensors ready)
  - RUNNING → ALERT (on threshold crossing)
  - RUNNING → ERROR (on unrecoverable failure)
  - ALERT → RUNNING (on conditions normal + hysteresis)
  - ALERT → ERROR (on persistent alert + timeout)
  - ERROR → RECOVERY (on backoff timer)
  - RECOVERY → STANDBY (on successful recovery)
  - RECOVERY → ERROR (on recovery failure)
  - ERROR → ERROR (any state to itself = re-entry)
- `transitionTo()`: validate transition, log via SerialManager, update state
- `getStateName()`: return state as string via stateToString()
- Timer: track `state_enter_time` via `esp_timer_get_time()`
</action>

<acceptance_criteria>
- StateMachine validates all 10 transitions per matrix above
- `transitionTo(ERROR)` from RUNNING works when unrecoverable
- `transitionTo(RECOVERY)` auto-transitions after backoff timer
- `transitionTo(STANDBY)` from RECOVERY works on successful recovery
- State enter time tracked correctly
- Invalid transitions return false without changing state
- `pio run` compiles
</acceptance_criteria>

### Task 2: Create error handler with auto-recovery

Implement the error detection and recovery system with exponential backoff.

<read_first>
- src/core/state_machine.h
- src/config/sampling_config.h
</read_first>

<action>
Write `src/core/error_handler.h`:
- Class `ErrorHandler`:
  - `void reportError(const char* source, const char* message)` — log and escalate
  - `void clearError(const char* source)` — clear specific error
  - `bool shouldRecover()` — check if backoff period has elapsed
  - `uint32_t getBackoffMs()` — return current backoff interval
  - `void resetBackoff()` — reset retry count
  - Private: `int retry_count`, `uint32_t last_error_ms`, `char last_source[32]`, `char last_message[64]`

Write `src/core/error_handler.cpp`:
- `reportError()`:
  - Log via SerialManager::printError()
  - Increment retry_count
  - Record last_error_ms via esp_timer_get_time()
  - Store source and message
- `shouldRecover()`:
  - Compute `elapsed = now - last_error_ms`
  - Return true if elapsed >= getBackoffMs()
- `getBackoffMs()`:
  - Compute: `BACKOFF_BASE_MS * (2 ^ retry_count)` (exponential backoff)
  - Cap at 60000ms (1 minute max)
  - If retry_count >= BACKOFF_RETRIES, return UINT32_MAX (never recover)
- `resetBackoff()`: set retry_count = 0
- `clearError()`: reset specific error, decrement retry_count if > 0
</action>

<acceptance_criteria>
- `reportError()` increments retry_count and logs via SerialManager
- `getBackoffMs()` returns exponential: 1000, 2000, 4000, 8000, ...
- Backoff caps at 60000ms
- After BACKOFF_RETRIES (3) retries, `shouldRecover()` returns false
- `resetBackoff()` resets retry_count to 0
- `pio run` compiles
</acceptance_criteria>

### Task 3: Create sensor manager with FreeRTOS tasks

Implement the sensor manager that orchestrates all sensors via FreeRTOS tasks with queues and mutex.

<read_first>
- src/hal/isensor.h
- src/hal/sensor_reading.h
- src/drivers/sensor/dht22_driver.h
- src/drivers/sensor/mq9_driver.h
- src/drivers/sensor/ky026_driver.h
- src/drivers/sensor/calibration.h
- src/config/pins_config.h
- src/config/sampling_config.h
- src/core/state_machine.h
</read_first>

<action>
Write `src/managers/sensor_manager.h`:
- Class `SensorManager`:
  - `void init()` — create drivers, configure ADC, set up interrupts
  - `void startTasks()` — spawn FreeRTOS tasks
  - `SensorReading getLatestReading(SensorType type)` — get last known reading
  - `QueueHandle_t getReadingQueue()` — expose queue for consumers
  - Private: `DHT22Driver* dht22`, `MQ9Driver* mq9`, `KY026Driver* ky026`
  - Private: `SensorCalibration* cal_dht22`, `SensorCalibration* cal_mq9`, `SensorCalibration* cal_ky026`
  - Private: `QueueHandle_t sensor_queue`, `SemaphoreHandle_t i2c_mutex`
  - Private: `TaskHandle_t task_dht22`, `TaskHandle_t task_mq9`, `TaskHandle_t task_ky026`
  - Static task functions: `static void taskDHT22(void* pvParams)`, etc.

Write `src/managers/sensor_manager.cpp`:
- `init()`:
  - Create i2c_mutex as `xSemaphoreCreateMutex()` (shared resource protection)
  - Create sensor_queue with item size `sizeof(SensorReading)`, length 20
  - Instantiate DHT22Driver(PIN_DHT22)
  - Instantiate MQ9Driver(ADC1_CHANNEL_6, GPIO_NUM_18) — power pin
  - Instantiate KY026Driver(PIN_KY026_DIGITAL, ADC1_CHANNEL_7)
  - Create SensorCalibration instances with CALIBRATION_SAMPLES
- `startTasks()`:
  - `xTaskCreatePinnedToCore(taskDHT22, "dht22_task", 4096, this, 2, &task_dht22, 1)` — priority 2 (normal)
  - `xTaskCreatePinnedToCore(taskMQ9, "mq9_task", 4096, this, 3, &task_mq9, 1)` — priority 3 (high)
  - `xTaskCreatePinnedToCore(taskKY026, "ky026_task", 2048, this, 4, &task_ky026, 1)` — priority 4 (critical)
- `taskDHT22()`:
  - Loop with vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_DHT22_MS))
  - Take i2c_mutex, call dht22->read(), give mutex
  - Add to calibration
  - Send to queue: `xQueueSend(sensor_queue, &reading, portMAX_DELAY)`
- `taskMQ9()`: similar, with SAMPLE_INTERVAL_MQ9_MS
- `taskKY026()`: similar, with SAMPLE_INTERVAL_KY026_MS (prioritized due to interrupt)
- `getLatestReading()`: return stored last reading per type

Write `src/managers/task_manager.h`:
- Class `TaskManager`:
  - `void init(SensorManager* sm, StateMachine* sm_state, ErrorHandler* eh)`
  - `void startMonitorTask()` — monitoring/health check task
  - Private: static `void monitorTask(void* pvParams)` — periodic health check

Write `src/managers/task_manager.cpp`:
- `monitorTask()`:
  - Priority 1 (low)
  - Every 10s: check all sensor statuses, print summary
  - If any sensor in ERROR for > 30s: escalate to StateMachine
</action>

<acceptance_criteria>
- SensorManager creates all 3 sensor drivers
- 3 FreeRTOS tasks created with correct priorities (4, 3, 2)
- I2C mutex protects shared resource access
- Queue passes SensorReading between tasks
- DHT22 task runs every 2000ms, MQ-9 every 1000ms, KY-026 every 200ms
- Task priorities: KY-026=4 (critical), MQ-9=3 (high), DHT22=2 (normal), monitor=1 (low)
- `pio run` compiles without errors
</acceptance_criteria>

### Task 4: Rewrite main.cpp with full integration

Replace main.cpp with the complete integrated system using SensorManager, StateMachine, and ErrorHandler.

<read_first>
- src/managers/sensor_manager.h
- src/managers/task_manager.h
- src/core/state_machine.h
- src/core/error_handler.h
- src/services/serial_manager.h
</read_first>

<action>
Rewrite `src/main.cpp`:
- `app_main()`:
  1. Initialize NVS: `nvs_flash_init()`
  2. Initialize SerialManager at 115200
  3. Create StateMachine — set alert thresholds from config
  4. Create ErrorHandler
  5. Create SensorManager — call `init()`
  6. Create TaskManager — pass SensorManager, StateMachine, ErrorHandler
  7. StateMachine::transitionTo(STANDBY) — after init
  8. SensorManager::startTasks() — spawn all sensor tasks
  9. StateMachine::transitionTo(RUNNING)
  10. TaskManager::startMonitorTask()
  11. Main loop:
      - Receive from sensor_queue periodically
      - Print readings via SerialManager
      - Check for alert conditions via StateMachine thresholds
      - If alert: StateMachine::transitionTo(ALERT)
      - If error: ErrorHandler::reportError(), StateMachine::transitionTo(ERROR)
      - Handle recovery: if ErrorHandler::shouldRecover(), call transitionTo(RECOVERY)
      - vTaskDelay(pdMS_TO_TICKS(100)) — yield to RTOS
</action>

<acceptance_criteria>
- `app_main()` initializes all managers in correct order
- State machine transitions through: INIT → STANDBY → RUNNING
- All 3 sensor tasks running concurrently
- Queue delivers readings to main loop
- Alert conditions trigger state transitions
- Error conditions trigger error handler + state machine
- Recovery loop works with exponential backoff
- `pio run` compiles without errors
- Flashing to ESP32 shows all 3 sensor readings + state transitions over serial
</acceptance_criteria>

## Verification Criteria

1. `pio run` exits 0 with no warnings
2. Flash to ESP32: all 3 sensors report readings over serial
3. State transitions visible: INIT → STANDBY → RUNNING
4. Alert state triggers when thresholds crossed (simulate with heat/gas)
5. Error recovery works (disconnect/reconnect sensor)
6. All 6 positive and negative acceptance criteria verified

## Must Haves

- [x] 3 FreeRTOS sensor tasks running concurrently with correct priorities
- [x] State machine transitions through all 6 states
- [x] Queue-based communication between sensor tasks and main loop
- [x] Mutex-protected I2C access
- [x] Error detection and auto-recovery with exponential backoff
