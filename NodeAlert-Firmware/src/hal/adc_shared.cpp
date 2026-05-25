/**
 * @file adc_shared.cpp
 * @brief Shared ADC1 unit handle implementation
 *
 * The static handle is initialised once on the first call and reused
 * on all subsequent calls, respecting the ESP-IDF constraint that only
 * one handle can exist per ADC unit.
 */

#include "adc_shared.h"

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static bool s_initialised = false;

esp_err_t nodealert_adc1_init(adc_oneshot_unit_handle_t *out_handle)
{
    if (!s_initialised) {
        adc_oneshot_unit_init_cfg_t cfg = {
            .unit_id   = ADC_UNIT_1,
            .clk_src   = (adc_oneshot_clk_src_t)0,
            .ulp_mode  = ADC_ULP_MODE_DISABLE,
        };

        esp_err_t err = adc_oneshot_new_unit(&cfg, &s_adc1_handle);
        if (err != ESP_OK) {
            return err;
        }
        s_initialised = true;
    }

    *out_handle = s_adc1_handle;
    return ESP_OK;
}
