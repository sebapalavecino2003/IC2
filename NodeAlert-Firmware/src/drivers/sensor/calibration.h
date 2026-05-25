/**
 * @file calibration.h
 * @brief Reusable calibration and signal processing module
 *
 * Provides statistical filtering utilities for sensor reading
 * stabilisation: moving average, median filtering, and stability
 * detection. Uses a fixed-capacity ring buffer to avoid unbounded
 * memory allocation at runtime.
 *
 * Typical usage:
 * @code
 *   SensorCalibration calib(10);
 *   calib.addSample(reading);
 *   float avg = calib.getMovingAverage();
 *   bool stable = calib.isStable(5.0f);
 * @endcode
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

class SensorCalibration {
public:
    /**
     * @brief Construct a calibration filter
     * @param window_size Maximum number of samples to retain
     */
    explicit SensorCalibration(size_t window_size);

    /**
     * @brief Add a sample to the ring buffer
     *
     * If the buffer is full, the oldest sample is overwritten.
     * @param value New sensor reading to add
     */
    void addSample(float value);

    /**
     * @brief Compute the moving average of buffered samples
     * @return Arithmetic mean of all samples in the buffer
     */
    float getMovingAverage() const;

    /**
     * @brief Compute the median of buffered samples
     *
     * Sorts a copy of the buffer and returns the middle value.
     * For even sample counts, returns the average of the two central values.
     * @return Median value
     */
    float getMedian() const;

    /**
     * @brief Check if readings are within a variance threshold
     *
     * A reading set is considered stable when every sample deviates from
     * the mean by less than the threshold.
     * @param variance_threshold Maximum allowed absolute deviation from mean
     * @return true if all samples are within threshold, false otherwise
     */
    bool isStable(float variance_threshold) const;

    /**
     * @brief Clear all samples from the buffer
     */
    void reset();

    /**
     * @brief Get the number of samples currently in the buffer
     * @return Number of samples stored
     */
    size_t getCount() const { return buffer.size(); }

private:
    std::vector<float>  buffer;         ///< Ring buffer of samples
    size_t              max_samples;    ///< Maximum buffer capacity
    size_t              current_index;  ///< Next write position in the buffer
};
