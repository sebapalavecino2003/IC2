/**
 * @file state_machine.cpp
 * @brief State machine implementation with transition validation
 *
 * Implements the 6-state finite state machine with 10 valid transitions.
 * All state changes are logged via SerialManager for debug traceability.
 */

#include "state_machine.h"
#include "services/serial_manager.h"
#include "esp_timer.h"

/* ========================================================================== */
/* Construction                                                               */
/* ========================================================================== */

StateMachine::StateMachine()
    : current_state(SystemState::INIT)
    , previous_state(SystemState::INIT)
    , state_enter_time((uint64_t)esp_timer_get_time())
    , alert_temp_high(50.0f)
    , alert_gas_high(2000.0f)
{
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

SystemState StateMachine::getCurrentState() const
{
    return current_state;
}

bool StateMachine::transitionTo(SystemState new_state)
{
    if (!isTransitionValid(current_state, new_state)) {
        // Invalid transition — log and reject
        SerialManager::printError("StateMachine",
            "Invalid transition rejected");
        return false;
    }

    // Record previous state and update
    previous_state   = current_state;
    current_state    = new_state;
    state_enter_time = (uint64_t)esp_timer_get_time();

    // Log the transition via SerialManager
    SerialManager::printState(getStateName());

    return true;
}

bool StateMachine::isTransitionValid(SystemState from, SystemState to) const
{
    // Self-transition (re-entry) is always valid
    if (from == to) return true;

    switch (from) {
        case SystemState::INIT:
            return (to == SystemState::STANDBY);

        case SystemState::STANDBY:
            return (to == SystemState::RUNNING);

        case SystemState::RUNNING:
            return (to == SystemState::ALERT ||
                    to == SystemState::ERROR);

        case SystemState::ALERT:
            return (to == SystemState::RUNNING ||
                    to == SystemState::ERROR);

        case SystemState::ERROR:
            return (to == SystemState::RECOVERY);

        case SystemState::RECOVERY:
            return (to == SystemState::STANDBY ||
                    to == SystemState::ERROR);

        default:
            return false;
    }
}

const char* StateMachine::getStateName() const
{
    return stateToString(current_state);
}

void StateMachine::setAlertThresholds(float temp_high, float gas_high)
{
    alert_temp_high = temp_high;
    alert_gas_high  = gas_high;
}

bool StateMachine::isInStateLongerThanUs(uint64_t duration_us) const
{
    uint64_t now = (uint64_t)esp_timer_get_time();
    return (now - state_enter_time) > duration_us;
}
