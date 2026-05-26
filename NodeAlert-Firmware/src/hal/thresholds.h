#pragma once

#include <cstdint>

struct ThresholdConfig {
    float    temp_warning;
    float    temp_critical;
    float    gas_warning;
    float    gas_critical;
    float    flame_threshold;
    float    humidity_low;
    float    humidity_high;
    uint32_t hysteresis_time_ms;
    float    hysteresis_delta_pct;
};

static RTC_DATA_ATTR ThresholdConfig g_thresholds = {
    .temp_warning        = 35.0f,
    .temp_critical       = 45.0f,
    .gas_warning         = 200.0f,
    .gas_critical        = 300.0f,
    .flame_threshold     = 0.5f,
    .humidity_low        = 20.0f,
    .humidity_high       = 80.0f,
    .hysteresis_time_ms  = 3000,
    .hysteresis_delta_pct = 0.10f,
};
