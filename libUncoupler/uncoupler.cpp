#include "uncoupler.h"
#include <cmath>
#include <numeric>

namespace Uncoupler {

SensorUncoupler::SensorUncoupler(size_t gravity_filter_size)
    : m_gx_offset(0.0f), m_gy_offset(0.0f), m_gz_offset(0.0f),
      m_gyroCalibrationEnabled(false),
      m_gravity_vector{0.0f, 0.0f, 1.0f}, // Initial assumption: gravity points down (z-axis)
      m_gravity_magnitude(9.81f),         // Initial gravity magnitude (standard Earth gravity)
      m_gravity_filter_size(gravity_filter_size)
{
    // Constructor initializes members
}

void SensorUncoupler::setGyroCalibrationOffsets(float gx_offset, float gy_offset, float gz_offset) {
    m_gx_offset = gx_offset;
    m_gy_offset = gy_offset;
    m_gz_offset = gz_offset;
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
    
    // Set gravity components in result
    result.grav_x = m_gravity_vector[0] * m_gravity_magnitude;
    result.grav_y = m_gravity_vector[1] * m_gravity_magnitude;
    result.grav_z = m_gravity_vector[2] * m_gravity_magnitude;
    
    // Calculate linear acceleration (remove gravity component)
    result.ax_linear = result.ax_raw - result.grav_x;
    result.ay_linear = result.ay_raw - result.grav_y;
    result.az_linear = result.az_raw - result.grav_z;
    
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
        
        // Update gravity magnitude estimate
        m_gravity_magnitude = magnitude;
        
        // Normalize to get the direction vector
        if (magnitude > 0.1f) {  // Avoid division by zero or very small values
            m_gravity_vector[0] = avg_ax / magnitude;
            m_gravity_vector[1] = avg_ay / magnitude;
            m_gravity_vector[2] = avg_az / magnitude;
        }
    }
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

void SensorUncoupler::enableGyroCalibration(bool enable) {
    m_gyroCalibrationEnabled = enable;
}

bool SensorUncoupler::isGyroCalibrationEnabled() const {
    return m_gyroCalibrationEnabled;
}

} // namespace Uncoupler
