#include "hand.h"

namespace Hand {

HandTracker::HandTracker() 
    : m_accel{0.0f, 0.0f, 0.0f}, m_gyro{0.0f, 0.0f, 0.0f}, 
      m_velocity{0.0f, 0.0f, 0.0f}, m_firstUpdate(true)
{
    // Initialize the timestamp
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void HandTracker::update(const std::unordered_map<std::string, int>& data) {
    // Get current time for velocity calculation
    auto currentTime = std::chrono::steady_clock::now();
    
    // Save previous acceleration values for velocity calculation
    Vector3D prevAccel = m_accel;
    
    // Update raw data
    m_accel.x = static_cast<float>(data.at("ax"));
    m_accel.y = static_cast<float>(data.at("ay"));
    m_accel.z = static_cast<float>(data.at("az"));
    
    m_gyro.x = static_cast<float>(data.at("gx"));
    m_gyro.y = static_cast<float>(data.at("gy"));
    m_gyro.z = static_cast<float>(data.at("gz"));
    
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

} // namespace Hand 