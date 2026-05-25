/**
 * @file calibration.cpp
 * @brief Calibration and signal processing implementation
 *
 * Implements a ring-buffer-based moving average, median filter,
 * and stability detector for sensor reading noise reduction.
 */

#include "calibration.h"
#include <algorithm>

/* ========================================================================== */
/* Construction                                                               */
/* ========================================================================== */

SensorCalibration::SensorCalibration(size_t window_size)
    : max_samples(window_size > 0 ? window_size : 1)
    , current_index(0)
{
    // Pre-allocate storage to avoid repeated reallocation during addSample()
    buffer.reserve(max_samples);
}

/* ========================================================================== */
/* Sample management                                                          */
/* ========================================================================== */

void SensorCalibration::addSample(float value)
{
    if (buffer.size() < max_samples) {
        // Buffer not yet full — append
        buffer.push_back(value);
    } else {
        // Buffer full — overwrite at current index (ring behaviour)
        buffer[current_index] = value;
    }
    current_index = (current_index + 1) % max_samples;
}

void SensorCalibration::reset()
{
    buffer.clear();
    current_index = 0;
}

/* ========================================================================== */
/* Statistical filters                                                        */
/* ========================================================================== */

float SensorCalibration::getMovingAverage() const
{
    if (buffer.empty()) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (float v : buffer) {
        sum += v;
    }
    return sum / (float)buffer.size();
}

float SensorCalibration::getMedian() const
{
    if (buffer.empty()) {
        return 0.0f;
    }

    // Sort a copy of the buffer
    std::vector<float> sorted = buffer;
    std::sort(sorted.begin(), sorted.end());

    size_t n = sorted.size();
    size_t mid = n / 2;

    if (n % 2 == 0) {
        // Even count — average of two middle values
        return (sorted[mid - 1] + sorted[mid]) / 2.0f;
    }

    // Odd count — middle value
    return sorted[mid];
}

bool SensorCalibration::isStable(float variance_threshold) const
{
    if (buffer.size() < 2) {
        return false;   // need at least 2 samples for meaningful check
    }

    float mean = getMovingAverage();
    for (float v : buffer) {
        float dev = v - mean;
        if (dev < 0.0f) dev = -dev;
        if (dev > variance_threshold) {
            return false;
        }
    }

    return true;
}
