/**
 * @file state_machine.h
 * @brief System state machine with transition validation
 *
 * Implements a 6-state finite state machine for the NodeAlert IoT system:
 *   INIT → STANDBY → RUNNING ↔ ALERT → RECOVERY → RUNNING
 *                      ↓
 *                    ERROR → RECOVERY
 *
 * All 10 valid transitions are enforced via isTransitionValid().
 * Self-transition (re-entry) is always permitted for any state.
 */

#pragma once

#include "core/system_core.h"
#include <cstdint>

class StateMachine {
public:
    /**
     * @brief Construct a StateMachine in INIT state
     */
    StateMachine();

    /**
     * @brief Get the current system state
     * @return Current SystemState enum value
     */
    SystemState getCurrentState() const;

    /**
     * @brief Attempt a state transition
     *
     * Validates the transition via isTransitionValid(), then executes it.
     * On success: prints the transition via SerialManager and records
     * the enter time via esp_timer_get_time().
     *
     * @param new_state The target state to transition to
     * @return true if transition was valid and executed, false otherwise
     */
    bool transitionTo(SystemState new_state);

    /**
     * @brief Check if a transition between two states is allowed
     *
     * Valid transitions (10 total):
     *   1.  INIT    → STANDBY  (on successful init)
     *   2.  STANDBY → RUNNING  (on all sensors ready)
     *   3.  RUNNING → ALERT    (on threshold crossing)
     *   4.  RUNNING → ERROR    (on unrecoverable failure)
     *   5.  ALERT   → RUNNING  (on conditions normal + hysteresis)
     *   6.  ALERT   → ERROR    (on persistent alert + timeout)
     *   7.  ERROR   → RECOVERY (on backoff timer)
     *   8.  RECOVERY → STANDBY (on successful recovery)
     *   9.  RECOVERY → ERROR   (on recovery failure)
     *   10. any     → any      (self-transition = re-entry)
     *
     * @param from Current state
     * @param to   Desired target state
     * @return true if the transition is allowed
     */
    bool isTransitionValid(SystemState from, SystemState to) const;

    /**
     * @brief Get human-readable name of the current state
     * @return C-string state name (e.g., "INIT", "RUNNING")
     */
    const char* getStateName() const;

    /**
     * @brief Configure alert trigger thresholds
     *
     * @param temp_high Temperature threshold in °C
     * @param gas_high  Gas concentration ratio threshold
     */
    void setAlertThresholds(float temp_high, float gas_high);

    /**
     * @brief Get the configured temperature alert threshold
     * @return Threshold in °C
     */
    float getAlertTempHigh() const { return alert_temp_high; }

    /**
     * @brief Get the configured gas alert threshold
     * @return Gas concentration ratio threshold
     */
    float getAlertGasHigh() const { return alert_gas_high; }

    /**
     * @brief Get the time when the current state was entered
     * @return Microseconds since boot (from esp_timer_get_time())
     */
    uint64_t getStateEnterTimeUs() const { return state_enter_time; }

    /**
     * @brief Check if the system has been in its current state
     *        longer than the specified duration (in microseconds)
     * @param duration_us Duration to compare against
     * @return true if current state duration exceeds duration_us
     */
    bool isInStateLongerThanUs(uint64_t duration_us) const;

private:
    SystemState current_state;       ///< Current system state
    SystemState previous_state;      ///< Previous system state
    uint64_t    state_enter_time;    ///< esp_timer_get_time() when state was entered (us)
    float       alert_temp_high;     ///< Temperature alert threshold (°C)
    float       alert_gas_high;      ///< Gas concentration alert threshold
};
