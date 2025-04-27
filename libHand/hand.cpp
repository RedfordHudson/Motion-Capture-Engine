#include "hand.h"

namespace Hand {

HandTracker::HandTracker() 
    : m_accel{0.0f, 0.0f, 0.0f}, m_gyro{0.0f, 0.0f, 0.0f} 
{
    // Nothing to initialize
}

void HandTracker::update(const std::unordered_map<std::string, int>& data) {
    // Simply store raw data without any processing
    m_accel.x = static_cast<float>(data.at("ax"));
    m_accel.y = static_cast<float>(data.at("ay"));
    m_accel.z = static_cast<float>(data.at("az"));
    
    m_gyro.x = static_cast<float>(data.at("gx"));
    m_gyro.y = static_cast<float>(data.at("gy"));
    m_gyro.z = static_cast<float>(data.at("gz"));
}

Vector3D HandTracker::getAcceleration() const {
    return m_accel;
}

Vector3D HandTracker::getGyroscope() const {
    return m_gyro;
}

} // namespace Hand 