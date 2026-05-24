/**
 * @file sampling_config.h
 * @brief Sampling intervals, thresholds, and system timing constants
 *
 * All time values are in milliseconds unless otherwise noted.
 * Adjust thresholds based on environmental calibration.
 */

#pragma once

#include <stdint.h>

/* ========================================================================== */
/* Sampling Intervals (ms)                                                    */
/* ========================================================================== */
#define SAMPLE_INTERVAL_DHT22_MS       2000
#define SAMPLE_INTERVAL_MQ9_MS         1000
#define SAMPLE_INTERVAL_KY026_MS       200

/* ========================================================================== */
/* Temperature Thresholds (°C)                                                */
/* ========================================================================== */
#define TEMP_THRESHOLD_HIGH_C          50.0f
#define TEMP_THRESHOLD_LOW_C          -10.0f

/* ========================================================================== */
/* Humidity Thresholds (%)                                                    */
/* ========================================================================== */
#define HUM_THRESHOLD_HIGH_PCT         90.0f

/* ========================================================================== */
/* Gas Thresholds (raw ADC value, 0–4095)                                     */
/* ========================================================================== */
#define GAS_THRESHOLD_HIGH             2000

/* ========================================================================== */
/* Hysteresis — Prevents rapid on/off cycling near thresholds                 */
/* ========================================================================== */
#define HYSTERESIS_TEMP                3.0f
#define HYSTERESIS_GAS                 200

/* ========================================================================== */
/* Retry / Backoff                                                           */
/* ========================================================================== */
#define BACKOFF_RETRIES                3
#define BACKOFF_BASE_MS                1000

/* ========================================================================== */
/* Watchdog                                                                   */
/* ========================================================================== */
#define WATCHDOG_TIMEOUT_MS            10000
