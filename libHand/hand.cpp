#include "hand.h"

namespace Hand {

HandTracker::HandTracker() 
    : m_accel{0.0f, 0.0f, 0.0f}, m_gyro{0.0f, 0.0f, 0.0f}, 
      m_velocity{0.0f, 0.0f, 0.0f}, m_firstUpdate(true),
      m_accelOffset{0.0f, 0.0f, 0.0f}, m_gyroOffset{0.0f, 0.0f, 0.0f},
      m_calibrationEnabled(false)
{
    // Initialize the timestamp
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void HandTracker::update(const std::unordered_map<std::string, int>& data) {
    // Get current time for velocity calculation
    auto currentTime = std::chrono::steady_clock::now();
    
    // Save previous acceleration values for velocity calculation
    Vector3D prevAccel = m_accel;
    
    // Get raw data
    int raw_ax = data.at("ax");
    int raw_ay = data.at("ay");
    int raw_az = data.at("az");
    int raw_gx = data.at("gx");
    int raw_gy = data.at("gy");
    int raw_gz = data.at("gz");
    
    // Apply calibration if enabled
    if (m_calibrationEnabled) {
        raw_ax = applyCalibratedOffset(raw_ax, m_accelOffset.x);
        raw_ay = applyCalibratedOffset(raw_ay, m_accelOffset.y);
        raw_az = applyCalibratedOffset(raw_az, m_accelOffset.z);
        raw_gx = applyCalibratedOffset(raw_gx, m_gyroOffset.x);
        raw_gy = applyCalibratedOffset(raw_gy, m_gyroOffset.y);
        raw_gz = applyCalibratedOffset(raw_gz, m_gyroOffset.z);
    }
    
    // Update data with calibrated values
    m_accel.x = static_cast<float>(raw_ax);
    m_accel.y = static_cast<float>(raw_ay);
    m_accel.z = static_cast<float>(raw_az);
    
    m_gyro.x = static_cast<float>(raw_gx);
    m_gyro.y = static_cast<float>(raw_gy);
    m_gyro.z = static_cast<float>(raw_gz);
    
    // Update velocity based on acceleration data
    updateVelocity(prevAccel, currentTime);
}

void HandTracker::updateVelocity(const Vector3D& prevAccel, const std::chrono::steady_clock::time_point& currentTime) {
    // Calculate time delta in seconds
    if (!m_firstUpdate) {
        std::chrono::duration<float> deltaTime = currentTime - m_lastUpdateTime;
        float dt = deltaTime.count();
        
        // Basic velocity calculation using trapezoidal integration:
        // v(t+dt) = v(t) + (a(t) + a(t+dt))/2 * dt
        m_velocity.x += (prevAccel.x + m_accel.x) * 0.5f * dt;
        m_velocity.y += (prevAccel.y + m_accel.y) * 0.5f * dt;
        m_velocity.z += (prevAccel.z + m_accel.z) * 0.5f * dt;
    } else {
        m_firstUpdate = false;
    }
    
    // Update timestamp
    m_lastUpdateTime = currentTime;
}

Vector3D HandTracker::getAcceleration() const {
    return m_accel;
}

Vector3D HandTracker::getGyroscope() const {
    return m_gyro;
}

Vector3D HandTracker::getVelocity() const {
    return m_velocity;
}

void HandTracker::setCalibrationOffsets(float ax_offset, float ay_offset, float az_offset,
                                       float gx_offset, float gy_offset, float gz_offset) {
    m_accelOffset.x = ax_offset;
    m_accelOffset.y = ay_offset;
    m_accelOffset.z = az_offset;
    m_gyroOffset.x = gx_offset;
    m_gyroOffset.y = gy_offset;
    m_gyroOffset.z = gz_offset;
}

int HandTracker::applyCalibratedOffset(int value, float offset) const {
    // Subtract the offset from the raw value
    // Converting offset to int for consistent arithmetic with the raw value
    return value - static_cast<int>(offset);
}

void HandTracker::enableCalibration(bool enable) {
    m_calibrationEnabled = enable;
}

bool HandTracker::isCalibrationEnabled() const {
    return m_calibrationEnabled;
}

} // namespace Hand 