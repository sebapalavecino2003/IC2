/**
 * @file system_core.h
 * @brief System state machine definitions
 *
 * Defines the top-level system states for the NodeAlert IoT firmware.
 * The system transitions through these states during normal operation:
 *
 *   INIT → STANDBY → RUNNING ↔ ALERT → RECOVERY → RUNNING
 *                      ↓
 *                    ERROR → RECOVERY
 */

#pragma once

#include <cstdint>

/* ========================================================================== */
/* SystemState — Top-level finite state machine                               */
/* ========================================================================== */
enum class SystemState : uint8_t {
    INIT,       ///< System booting, initializing components
    STANDBY,    ///< Initialized but not actively sampling
    RUNNING,    ///< Normal operation — sensors sampling, MQTT connected
    ALERT,      ///< Threshold crossed — hazard condition detected
    ERROR,      ///< Non-recoverable error — needs recovery
    RECOVERY    ///< Attempting to return to RUNNING after alert/error
};

/**
 * @brief Convert a SystemState to its human-readable string representation
 * @param state The state to convert
 * @return Pointer to a static C-string (e.g., "INIT", "RUNNING", "ALERT")
 */
const char* stateToString(SystemState state);
