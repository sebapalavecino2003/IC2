/**
 * @file error_handler.h
 * @brief Error detection and auto-recovery with exponential backoff
 *
 * Tracks sensor and system errors with retry counting and exponential
 * backoff timing. Supports recovery decisions based on elapsed time
 * and retry budget exhaustion.
 *
 * Backoff sequence (with BACKOFF_BASE_MS = 1000, BACKOFF_RETRIES = 3):
 *   Retry 0: 1000ms
 *   Retry 1: 2000ms
 *   Retry 2: 4000ms
 *   Retry 3: 8000ms (max before exhaustion)
 *   After 3: UINT32_MAX (never recover — system must be reset)
 *
 * All backoff values are capped at 60000ms (1 minute).
 */

#pragma once

#include <cstdint>

class ErrorHandler {
public:
    /**
     * @brief Construct an ErrorHandler with zero retry count
     */
    ErrorHandler();

    /**
     * @brief Report an error from a system component
     *
     * Logs the error via SerialManager::printError(), increments the
     * retry count, and records the current time for backoff computation.
     *
     * @param source  Component or driver name (e.g., "DHT22", "MQ-9")
     * @param message Descriptive error message
     */
    void reportError(const char* source, const char* message);

    /**
     * @brief Clear the error for a specific source
     *
     * Decrements retry_count if > 0, resets the source and message
     * buffers to empty.
     *
     * @param source Component name to clear
     */
    void clearError(const char* source);

    /**
     * @brief Check if the system should attempt recovery
     *
     * Compares elapsed time since the last error against the current
     * backoff interval (see getBackoffMs()).
     *
     * @return true if the backoff period has elapsed and recovery should proceed
     */
    bool shouldRecover();

    /**
     * @brief Get the current backoff interval in milliseconds
     *
     * Exponential backoff: BACKOFF_BASE_MS * 2^retry_count
     *   retry_count=0: 1000ms
     *   retry_count=1: 2000ms
     *   retry_count=2: 4000ms
     *   retry_count=3: 8000ms
     *   retry_count>=BACKOFF_RETRIES (3): UINT32_MAX (never recover)
     *
     * All values capped at 60000ms max.
     *
     * @return Current backoff interval in ms
     */
    uint32_t getBackoffMs();

    /**
     * @brief Reset retry count to zero (e.g., after successful recovery)
     */
    void resetBackoff();

    /**
     * @brief Get the current retry count
     * @return Number of errors reported since last reset
     */
    int getRetryCount() const { return retry_count; }

private:
    int         retry_count;        ///< Number of consecutive errors reported
    uint32_t    last_error_ms;      ///< System uptime of last error (ms)
    char        last_source[32];    ///< Last error source component name
    char        last_message[64];   ///< Last error message text
};
