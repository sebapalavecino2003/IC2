/**
 * @file error_handler.cpp
 * @brief Error handler implementation with exponential backoff recovery
 */

#include "error_handler.h"
#include "services/serial_manager.h"
#include "config/sampling_config.h"
#include "esp_timer.h"
#include <cstring>
#include <climits>

/* ========================================================================== */
/* Construction                                                               */
/* ========================================================================== */

ErrorHandler::ErrorHandler()
    : retry_count(0)
    , last_error_ms(0)
{
    last_source[0]  = '\0';
    last_message[0] = '\0';
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

void ErrorHandler::reportError(const char* source, const char* message)
{
    // Log via SerialManager
    SerialManager::printError(
        (source != nullptr) ? source : "ErrorHandler",
        (message != nullptr) ? message : "Unknown error");

    // Increment retry count
    retry_count++;

    // Record timestamp (ms since boot)
    last_error_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Store source and message (with bounds checking)
    if (source != nullptr) {
        size_t src_len = strlen(source);
        if (src_len >= sizeof(last_source)) {
            src_len = sizeof(last_source) - 1;
        }
        memcpy(last_source, source, src_len);
        last_source[src_len] = '\0';
    } else {
        last_source[0] = '\0';
    }

    if (message != nullptr) {
        size_t msg_len = strlen(message);
        if (msg_len >= sizeof(last_message)) {
            msg_len = sizeof(last_message) - 1;
        }
        memcpy(last_message, message, msg_len);
        last_message[msg_len] = '\0';
    } else {
        last_message[0] = '\0';
    }
}

void ErrorHandler::clearError(const char* source)
{
    // Only clear if the source matches (or if nullptr, clear all)
    if (source == nullptr ||
        strncmp(last_source, source, sizeof(last_source)) == 0) {
        // Decrement retry count if > 0
        if (retry_count > 0) {
            retry_count--;
        }
        last_source[0]  = '\0';
        last_message[0] = '\0';
        last_error_ms   = 0;
    }
}

bool ErrorHandler::shouldRecover()
{
    if (retry_count >= BACKOFF_RETRIES) {
        // Retry budget exhausted — never recover
        return false;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = now_ms - last_error_ms;
    uint32_t backoff = getBackoffMs();

    return (elapsed >= backoff);
}

uint32_t ErrorHandler::getBackoffMs()
{
    if (retry_count >= BACKOFF_RETRIES) {
        return UINT32_MAX;
    }

    // Exponential: BACKOFF_BASE_MS * 2^retry_count
    uint32_t backoff = BACKOFF_BASE_MS * (uint32_t)(1u << retry_count);

    // Cap at 1 minute
    if (backoff > 60000u) {
        backoff = 60000u;
    }

    return backoff;
}

void ErrorHandler::resetBackoff()
{
    retry_count    = 0;
    last_error_ms  = 0;
    last_source[0] = '\0';
    last_message[0] = '\0';
}
