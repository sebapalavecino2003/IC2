---
phase: 01-firmware-foundation-sensores
plan: 01-D-RTOS-INTEGRATION
subsystem: firmware
tags: [freertos, state-machine, error-handler, mutex, queue, task-priority, esp32, espidf]

# Dependency graph
requires:
  - phase: 01-A-SCAFFOLD
    provides: HAL interface (ISensor, SensorReading), config system, PlatformIO project
  - phase: 01-B-DHT22
    provides: DHT22 driver, SerialManager, SystemState enum
  - phase: 01-C-MQ9-KY026
    provides: MQ-9 driver, KY-026 driver, SensorCalibration, shared ADC module
provides:
  - StateMachine class with 6-state FSM and 10 validated transitions
  - ErrorHandler with exponential backoff auto-recovery
  - SensorManager orchestrating 3 FreeRTOS sensor tasks with queue + mutex
  - TaskManager with low-priority health monitor task
  - Fully integrated main.cpp with alert/error/recovery loop
affects:
  - Phase 02-mqtt-wifi (monitor task and state machine ready for connectivity)

# Tech tracking
tech-stack:
  added:
    - FreeRTOS xTaskCreatePinnedToCore for multi-task orchestration
    - xSemaphoreCreateMutex / xSemaphoreTake / xSemaphoreGive for shared resource protection
    - xQueueCreate / xQueueSend / xQueueReceive for inter-task communication
  patterns:
    - Static FreeRTOS task functions dispatching via void* pvParams to class instance
    - Exponential backoff with retry budget exhaustion (UINT32_MAX = never recover)
    - Hysteresis-based state normalisation to prevent threshold oscillation
    - Template getter pattern for type-safe driver access without RTTI

key-files:
  created:
    - NodeAlert-Firmware/src/core/state_machine.h
    - NodeAlert-Firmware/src/core/state_machine.cpp
    - NodeAlert-Firmware/src/core/error_handler.h
    - NodeAlert-Firmware/src/core/error_handler.cpp
    - NodeAlert-Firmware/src/managers/sensor_manager.h
    - NodeAlert-Firmware/src/managers/sensor_manager.cpp
    - NodeAlert-Firmware/src/managers/task_manager.h
    - NodeAlert-Firmware/src/managers/task_manager.cpp
  modified:
    - NodeAlert-Firmware/src/core/main.cpp (full rewrite)
    - NodeAlert-Firmware/src/config/pins_config.h (ADC channel fix)

key-decisions:
  - "Mutex named hw_mutex per locked decision D-07 (shared GPIO/ADC protection, NOT i2c_mutex)"
  - "Template getSensor<T>() method for type-safe driver access (D-04 requirement)"
  - "Task priorities: KY-026=4 (critical) > MQ-9=3 (high) > DHT22=2 (normal) > monitor=1 (low)"
  - "State transitions matrix with exactly 10 valid transitions as specified (including self-transition re-entry)"
  - "Exponential backoff: BACKOFF_BASE_MS × 2^retry_count, capped at 60000ms, retry budget = 3"
  - "Hysteresis thresholds: TEMP=3°C, GAS=200 ADC units prevent rapid on/off cycling"
  - "All 3 sensor tasks pinned to core 1 (core 0 reserved for WiFi/FreeRTOS idle)"

patterns-established:
  - "FreeRTOS task pattern: acquire mutex → read sensor → release mutex → calibrate → queue → delay"
  - "Monitor task pattern: periodic health check with persistent-error escalation (>30s → ERROR state)"
  - "Recovery state machine: ERROR → RECOVERY (backoff) → STANDBY (reset) → RUNNING (resume)"

requirements-completed:
  - RTOS-01
  - RTOS-02
  - RTOS-03
  - RTOS-04
  - RTOS-05
  - FWK-04
  - FWK-05

# Metrics
duration: 7min
completed: 2026-05-25
---

# Phase 1 Plan D: FreeRTOS Tasks + State Machine + Integration — Summary

**Integrated 6-state FSM with 10 validated transitions, exponential backoff error recovery, 3 concurrent FreeRTOS sensor tasks (KY-026=pri4, MQ-9=pri3, DHT22=pri2) with queue-based communication and mutex-protected GPIO/ADC access, plus a health monitor task, all wired through a fully rewritten main.cpp**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-25T00:26:03Z
- **Completed:** 2026-05-25T00:33:23Z
- **Tasks:** 4 (all type=auto)
- **Files modified:** 10 (8 created, 2 modified)

## Accomplishments

- **StateMachine (RTOS-04, FWK-04):** 6-state FSM (INIT → STANDBY → RUNNING ↔ ALERT → RECOVERY → RUNNING, with ERROR) enforcing exactly 10 valid transitions via isTransitionValid(). Self-transition (re-entry) always permitted. Transition logging via SerialManager, configurable alert thresholds with getters, and timing-based state duration checks.
- **ErrorHandler (RTOS-05, FWK-05):** Exponential backoff auto-recovery: reportError() logs, increments retry count, records timestamp; getBackoffMs() computes BASE × 2^retry_count (1000, 2000, 4000, 8000ms), capped at 60s; after BACKOFF_RETRIES (3) exhaustion returns UINT32_MAX (never recover). shouldRecover() checks elapsed ≥ backoff. resetBackoff() for successful recovery.
- **SensorManager (RTOS-01, RTOS-02, RTOS-03):** Owns DHT22Driver, MQ9Driver, KY026Driver with SensorCalibration per sensor. Creates `hw_mutex` (SemaphoreHandle) and `sensor_queue` (QueueHandle, 20 items of SensorReading). Spawns 3 FreeRTOS tasks pinned to core 1 with correct priorities (KY-026=4, MQ-9=3, DHT22=2). Each task loop: take mutex → read → give mutex → calibrate → queue. Template `getSensor<T>()` for type-safe driver access.
- **TaskManager:** init() wires SensorManager, StateMachine, ErrorHandler. startMonitorTask() spawns priority-1 task that prints health summary every 10s and escalates persistent errors (>30s) via StateMachine::transitionTo(ERROR).
- **Integrated main.cpp:** Full sequence: NVS → Serial → StateMachine INIT → ErrorHandler → SensorManager.init → TaskManager.init → STANDBY → startTasks() → RUNNING → startMonitorTask() → main loop. Main loop consumes queue (100ms timeout), detects alerts (RUNNING→ALERT), normalises with hysteresis (ALERT→RUNNING), detects errors (RUNNING→ERROR), and manages recovery (ERROR→RECOVERY→STANDBY→RUNNING).

## Task Commits

Each task was committed atomically:

1. **Task 1: Create complete state machine with all transitions** — `d1f1fb8` (feat)
2. **Task 2: Create error handler with auto-recovery** — `2d5b9f8` (feat)
3. **Task 3: Create sensor and task managers with FreeRTOS tasks** — `df5dcd3` (feat)
4. **Task 4: Rewrite main.cpp with full integration** — `fe35718` (feat)

## Files Created/Modified

- `NodeAlert-Firmware/src/core/state_machine.h` — 6-state FSM with transition validation, alert thresholds, enter-time tracking
- `NodeAlert-Firmware/src/core/state_machine.cpp` — 10-transition matrix, transitionTo() with SerialManager logging
- `NodeAlert-Firmware/src/core/error_handler.h` — ErrorHandler class with retry count, backoff, source/message buffers
- `NodeAlert-Firmware/src/core/error_handler.cpp` — reportError(), clearError(), shouldRecover(), getBackoffMs(), resetBackoff()
- `NodeAlert-Firmware/src/managers/sensor_manager.h` — SensorManager with drivers, calibrations, queue, mutex, template getter
- `NodeAlert-Firmware/src/managers/sensor_manager.cpp` — 3 FreeRTOS task loops with mutex-protected sensor reading and queuing
- `NodeAlert-Firmware/src/managers/task_manager.h` — TaskManager with init() and startMonitorTask()
- `NodeAlert-Firmware/src/managers/task_manager.cpp` — 10s health-check monitor, 30s error escalation
- `NodeAlert-Firmware/src/core/main.cpp` — Full system integration with alert/error/recovery management
- `NodeAlert-Firmware/src/config/pins_config.h` — Fixed PIN_MQ9_ADC_CHANNEL to ADC_CHANNEL_6

## Decisions Made

- **Mutex naming:** The shared mutex is `hw_mutex` as per locked decision D-07, NOT `i2c_mutex` (corrected during plan review). No sensor uses I2C — the mutex protects shared GPIO lines (DHT22 GPIO4) and the ADC1 unit (shared between MQ-9 KY-026).
- **getSensor<T>() template:** Implemented per D-04 requirement for type-safe access to concrete driver classes without RTTI. Uses `if constexpr` with `std::is_same_v` for compile-time dispatch. Returns `nullptr` for unsupported types.
- **Priority ordering:** Strictly per D-07: KY-026 (flame, critical) = priority 4, MQ-9 (gas, high) = priority 3, DHT22 (temp, normal) = priority 2, monitor (health, low) = priority 1.
- **State transition fidelity:** The 10-transition matrix matches the plan exactly, including self-transition re-entry for all states.
- **Hysteresis values:** Using TEMP=3.0°C and GAS=200 ADC units from sampling_config.h, matching the existing config constants.
- **Backoff parameters:** BACKOFF_BASE_MS=1000, BACKOFF_RETRIES=3, cap=60000ms from sampling_config.h. After 3 retries, shouldRecover() returns false permanently (system requires reset).
- **Main loop architecture:** Queue-driven with 100ms timeout, handling alert detection, error detection, hysteresis-based normalisation, and recovery sequencing in a single while(1) loop.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed PIN_MQ9_ADC_CHANNEL from legacy ADC1_CHANNEL_6 to ADC_CHANNEL_6**
- **Found during:** Task 3 (sensor_manager.cpp compilation)
- **Issue:** `pins_config.h` defined `PIN_MQ9_ADC_CHANNEL` as `ADC1_CHANNEL_6`, a legacy constant not available as a bare macro in ESP-IDF 6.x without specific ADC headers. The same issue was partially fixed in plan 01-C for `PIN_KY026_ADC_CHANNEL` (changed to `ADC_CHANNEL_7`) but `PIN_MQ9_ADC_CHANNEL` was missed.
- **Fix:** Changed `ADC1_CHANNEL_6` to `ADC_CHANNEL_6` in pins_config.h
- **Files modified:** `NodeAlert-Firmware/src/config/pins_config.h`
- **Verification:** `pio run` succeeds for both environments
- **Committed in:** `df5dcd3` (Task 3 commit)

**2. [Rule 2 - Missing Critical] Added getAlertTempHigh()/getAlertGasHigh() accessors to StateMachine**
- **Found during:** Task 4 (main.cpp main loop implementation)
- **Issue:** The plan specified `setAlertThresholds()` for configuring thresholds but did not include corresponding getter methods. The main loop needs to read these thresholds to check alert conditions (temperature vs. threshold, gas ratio vs. threshold).
- **Fix:** Added inline `getAlertTempHigh()` and `getAlertGasHigh()` accessors to StateMachine
- **Files modified:** `NodeAlert-Firmware/src/core/state_machine.h`
- **Verification:** Main loop correctly reads thresholds for alert detection; `pio run` succeeds
- **Committed in:** `d1f1fb8` (Task 1 commit, added during initial implementation)

---

**Total deviations:** 2 auto-fixed (1 blocking build issue, 1 missing critical functionality)
**Impact on plan:** Both fixes essential for correct compilation and functional alert detection. No scope creep.

## Issues Encountered

- **Pre-existing ADC constant mismatch:** `pins_config.h` had `PIN_MQ9_ADC_CHANNEL` defined using the legacy `ADC1_CHANNEL_6` constant, which is only available when specific ADC headers are included before the config header. This was partially addressed in plan 01-C (KY-026) but the MQ-9 constant was missed. The sensor_manager.cpp include chain (via driver headers) triggered the error because the ADC type system was exposed before the config.
- **Include dependency ordering:** `sensor_manager.cpp` must include its own header first (which includes the driver headers with ADC type definitions) before `pins_config.h` to ensure `ADC_CHANNEL_6` is resolved. This is a dependency ordering pattern that future consumers of pins_config.h should be aware of.

## Stub Tracking

- `sensor_manager.cpp`: `getSensor<T>()` returns `nullptr` for unsupported template instantiations — intentional, SFINAE-friendly fallback.
- `main.cpp`: Alert conditions use hard-coded threshold checks against StateMachine values — intentional, will be extended with dynamic threshold management in later phases.
- `task_manager.cpp`: Monitor task escalates errors to ERROR state but does not attempt automatic recovery from the monitor level — intentional, recovery is handled by the main loop's backoff logic.

## Threat Flags

None — all files operate at the sensor/firmware layer with no network endpoints, auth paths, or mutable data at trust boundaries. GPIO, ADC, and FreeRTOS primitives are hardware-mediated.

## Next Phase Readiness

- **Ready for Phase 2 (MQTT/WiFi):** The TaskManager monitor task provides a framework for connectivity health checks. The state machine ERROR and RECOVERY states can be extended for network loss. The main loop architecture supports adding MQTT publish/subscribe without restructuring.
- **Ready for Phase 3 (Automation):** The alert state and hysteresis mechanisms establish the local-autonomy pattern. Threshold getters/setters on StateMachine allow remote override of alert parameters via MQTT.
- All requirements RTOS-01 through RTOS-05, FWK-04, and FWK-05 are fulfilled.
- Phase 01 firmware foundation is complete — all 3 sensor drivers, state machine, error handling, and FreeRTOS task orchestration are in place and compiling.

## Self-Check: PASSED

- ✅ All 8 created files verified on disk
- ✅ All 4 commits confirmed in git log
- ✅ `pio run` succeeds for both `esp32dev` and `test` environments
- ✅ StateMachine validates 10 transitions with correct state tracking
- ✅ ErrorHandler computes exponential backoff (1000, 2000, 4000, 8000...) with cap at 60000ms
- ✅ SensorManager creates 3 FreeRTOS tasks with correct priorities (4, 3, 2)
- ✅ Mutex (hw_mutex) protects shared resource access
- ✅ Queue passes SensorReading between tasks
- ✅ TaskManager monitor task prints health summary every 10s
- ✅ main.cpp integrates all components with correct initialisation sequence

---

*Phase: 01-firmware-foundation-sensores*
*Completed: 2026-05-25*
