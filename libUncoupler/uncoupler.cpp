#include "uncoupler.h"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace Uncoupler {

SensorUncoupler::SensorUncoupler(size_t gravity_filter_size, float alpha)
    : m_gx_offset(0.0f), m_gy_offset(0.0f), m_gz_offset(0.0f),
      m_gyroCalibrationEnabled(false),
      m_gravity_vector{0.0f, 0.0f, 1.0f}, // Initial assumption: gravity points down (z-axis)
      m_gravity_magnitude(9.81f),         // Initial gravity magnitude (standard Earth gravity)
      m_filtered_gravity{0.0f, 0.0f, 9.81f}, // Initial filtered gravity
      m_alpha(alpha),
      m_gravity_filter_size(gravity_filter_size),
      m_prev_linear_accel{0.0f, 0.0f, 0.0f},
      m_filters_initialized(false)
{
    // Initialize filtered gravity to point down
    m_filtered_gravity[0] = 0.0f;
    m_filtered_gravity[1] = 0.0f;
    m_filtered_gravity[2] = 9.81f;
}

void SensorUncoupler::setGyroCalibrationOffsets(float gx_offset, float gy_offset, float gz_offset) {
    m_gx_offset = gx_offset;
    m_gy_offset = gy_offset;
    m_gz_offset = gz_offset;
}

void SensorUncoupler::setLowPassFilterAlpha(float alpha) {
    // Ensure alpha is in the valid range [0,1]
    m_alpha = std::max(0.0f, std::min(1.0f, alpha));
}

void SensorUncoupler::setGravityFilterSize(size_t size) {
    // Minimum size of 1 to avoid division by zero
    m_gravity_filter_size = std::max(size_t(1), size);
    
    // Resize history if needed
    if (m_ax_history.size() > m_gravity_filter_size) {
        while (m_ax_history.size() > m_gravity_filter_size) {
            m_ax_history.pop_front();
            m_ay_history.pop_front();
            m_az_history.pop_front();
        }
    }
}

UncoupledData SensorUncoupler::processData(const std::unordered_map<std::string, int>& sensorData) {
    UncoupledData result;
    
    // Get raw data
    int raw_ax = sensorData.at("ax");
    int raw_ay = sensorData.at("ay");
    int raw_az = sensorData.at("az");
    int raw_gx = sensorData.at("gx");
    int raw_gy = sensorData.at("gy");
    int raw_gz = sensorData.at("gz");
    
    // Store raw accelerometer data as-is (with gravity)
    result.ax_raw = static_cast<float>(raw_ax);
    result.ay_raw = static_cast<float>(raw_ay);
    result.az_raw = static_cast<float>(raw_az);
    
    // Apply calibration to gyroscope data if enabled
    if (m_gyroCalibrationEnabled) {
        result.gx_cal = static_cast<float>(applyGyroCalibration(raw_gx, m_gx_offset));
        result.gy_cal = static_cast<float>(applyGyroCalibration(raw_gy, m_gy_offset));
        result.gz_cal = static_cast<float>(applyGyroCalibration(raw_gz, m_gz_offset));
    } else {
        // Use raw values if calibration is disabled
        result.gx_cal = static_cast<float>(raw_gx);
        result.gy_cal = static_cast<float>(raw_gy);
        result.gz_cal = static_cast<float>(raw_gz);
    }
    
    // Update gravity vector estimation
    updateGravityEstimation(result.ax_raw, result.ay_raw, result.az_raw);
    
    // Set gravity components in result - use filtered gravity for smoother output
    result.grav_x = m_filtered_gravity[0];
    result.grav_y = m_filtered_gravity[1];
    result.grav_z = m_filtered_gravity[2];
    
    // Calculate linear acceleration (remove gravity component)
    float linear_ax = result.ax_raw - result.grav_x;
    float linear_ay = result.ay_raw - result.grav_y;
    float linear_az = result.az_raw - result.grav_z;
    
    // Initialize linear acceleration filters if needed
    if (!m_filters_initialized) {
        m_prev_linear_accel[0] = linear_ax;
        m_prev_linear_accel[1] = linear_ay;
        m_prev_linear_accel[2] = linear_az;
        m_filters_initialized = true;
    }
    
    // Apply additional low-pass filtering to linear acceleration for smoother output
    // Using a stronger filter (alpha/4) for linear acceleration to reduce noise
    applyLowPassFilter(m_prev_linear_accel[0], linear_ax, m_alpha * 0.25f);
    applyLowPassFilter(m_prev_linear_accel[1], linear_ay, m_alpha * 0.25f);
    applyLowPassFilter(m_prev_linear_accel[2], linear_az, m_alpha * 0.25f);
    
    // Store filtered linear acceleration
    result.ax_linear = m_prev_linear_accel[0];
    result.ay_linear = m_prev_linear_accel[1];
    result.az_linear = m_prev_linear_accel[2];
    
    return result;
}

int SensorUncoupler::applyGyroCalibration(int value, float offset) const {
    // Subtract the offset from the raw value
    // Converting offset to int for consistent arithmetic with the raw value
    return value - static_cast<int>(offset);
}

void SensorUncoupler::updateGravityEstimation(float ax, float ay, float az) {
    // Add new accelerometer values to history
    m_ax_history.push_back(ax);
    m_ay_history.push_back(ay);
    m_az_history.push_back(az);
    
    // Remove oldest values if we exceed the filter size
    if (m_ax_history.size() > m_gravity_filter_size) {
        m_ax_history.pop_front();
        m_ay_history.pop_front();
        m_az_history.pop_front();
    }
    
    // Calculate average values if we have enough data
    if (m_ax_history.size() >= 5) {  // Require at least 5 samples for a meaningful average
        // Calculate average of each component
        float avg_ax = std::accumulate(m_ax_history.begin(), m_ax_history.end(), 0.0f) / m_ax_history.size();
        float avg_ay = std::accumulate(m_ay_history.begin(), m_ay_history.end(), 0.0f) / m_ay_history.size();
        float avg_az = std::accumulate(m_az_history.begin(), m_az_history.end(), 0.0f) / m_az_history.size();
        
        // Calculate magnitude of the average acceleration vector
        float magnitude = std::sqrt(avg_ax * avg_ax + avg_ay * avg_ay + avg_az * avg_az);
        
        // Update gravity magnitude estimate (low-pass filtered for stability)
        applyLowPassFilter(m_gravity_magnitude, magnitude, m_alpha * 0.5f);
        
        // Normalize to get the direction vector
        if (magnitude > 0.1f) {  // Avoid division by zero or very small values
            // Calculate gravity direction
            float gx = avg_ax / magnitude;
            float gy = avg_ay / magnitude;
            float gz = avg_az / magnitude;
            
            // Update raw gravity vector
            m_gravity_vector[0] = gx;
            m_gravity_vector[1] = gy;
            m_gravity_vector[2] = gz;
            
            // Apply low-pass filter to gravity vector components for smoother output
            applyLowPassFilter(m_filtered_gravity[0], gx * m_gravity_magnitude, m_alpha);
            applyLowPassFilter(m_filtered_gravity[1], gy * m_gravity_magnitude, m_alpha);
            applyLowPassFilter(m_filtered_gravity[2], gz * m_gravity_magnitude, m_alpha);
        }
    }
}

void SensorUncoupler::applyLowPassFilter(float& value, float newValue, float alpha) {
    // Low pass filter: output = alpha * newValue + (1 - alpha) * previousOutput
    value = alpha * newValue + (1.0f - alpha) * value;
}

void SensorUncoupler::normalizeVector(float& x, float& y, float& z) {
    float magnitude = std::sqrt(x * x + y * y + z * z);
    if (magnitude > 0.0001f) {  // Avoid division by zero
        x /= magnitude;
        y /= magnitude;
        z /= magnitude;
    }
}

const float* SensorUncoupler::getGravityVector() const {
    return m_gravity_vector;
}

float SensorUncoupler::getGravityMagnitude() const {
    return m_gravity_magnitude;
}

void SensorUncoupler::enableGyroCalibration(bool enable) {
    m_gyroCalibrationEnabled = enable;
}

bool SensorUncoupler::isGyroCalibrationEnabled() const {
    return m_gyroCalibrationEnabled;
}

} // namespace Uncoupler
