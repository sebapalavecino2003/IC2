/**
 * @file system_core.cpp
 * @brief System state machine implementation
 */

#include "system_core.h"

const char* stateToString(SystemState state)
{
    switch (state) {
        case SystemState::INIT:     return "INIT";
        case SystemState::STANDBY:  return "STANDBY";
        case SystemState::RUNNING:  return "RUNNING";
        case SystemState::ALERT:    return "ALERT";
        case SystemState::ERROR:    return "ERROR";
        case SystemState::RECOVERY: return "RECOVERY";
        default:                    return "UNKNOWN";
    }
}
