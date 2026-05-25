/**
 * @file adc_shared.h
 * @brief Shared ADC unit handle for ESP-IDF oneshot mode
 *
 * ESP-IDF 6.x replaces the legacy adc1_get_raw() API with a handle-based
 * oneshot API (adc_oneshot_read). Only one handle can be created per
 * ADC unit. This module provides a shared ADC1 handle so that multiple
 * sensor drivers (MQ-9, KY-026) can read from different channels on the
 * same ADC unit without conflict.
 *
 * Usage:
 *   adc_oneshot_unit_handle_t adc_handle;
 *   ESP_ERROR_CHECK(nodealert_adc1_init(&adc_handle));
 *   adc_oneshot_config_channel(adc_handle, channel, &chan_cfg);
 *   adc_oneshot_read(adc_handle, channel, &raw);
 */

#pragma once

#include "esp_adc/adc_oneshot.h"

/**
 * @brief Initialise (or retrieve) the shared ADC1 unit handle
 *
 * The first call creates the ADC1 handle; subsequent calls return the
 * already-created handle. This is safe to call from multiple sensor
 * driver constructors.
 *
 * @param[out] out_handle Pointer to receive the ADC1 unit handle
 * @return ESP_OK on success, or an esp_err_t error code
 */
esp_err_t nodealert_adc1_init(adc_oneshot_unit_handle_t *out_handle);
