#include "isolator.h"
#include <algorithm>

namespace Isolator {

MotionIsolator::MotionIsolator() 
    : m_gravity{0.0f, 0.0f, 9.81f}  // Default gravity along z-axis
    , m_prevAccel{0.0f, 0.0f, 0.0f}
    , m_prevGyro{0.0f, 0.0f, 0.0f}
    , m_filteredAccel{0.0f, 0.0f, 0.0f}
    , m_alpha{0.95f}  // Default low-pass filter coefficient
    , m_beta{0.1f}    // Default high-pass filter coefficient
    , m_sampleRate{100.0f}  // Default 100Hz sample rate
    , m_lastTime{0.0f}
    , m_initialized{false}
    , m_bufferIndex{0}
{
    // Initialize acceleration buffer
    for (auto& accel : m_accelBuffer) {
        accel = Vector3D(0.0f, 0.0f, 0.0f);
    }
}

MotionIsolator::~MotionIsolator() {
    // No dynamic resources to clean up
}

void MotionIsolator::initialize(float sampleRate, float cutoffFreq) {
    m_sampleRate = sampleRate;
    
    // Calculate filter coefficients based on cutoff frequency
    // Alpha for low-pass (gravity estimation)
    float dt = 1.0f / sampleRate;
    float RC = 1.0f / (2.0f * M_PI * cutoffFreq);
    m_alpha = RC / (RC + dt);
    
    // Beta for high-pass (linear acceleration)
    m_beta = 1.0f - m_alpha;
    
    // Reset state
    reset();
    
    m_initialized = true;
}

void MotionIsolator::reset() {
    m_prevAccel = Vector3D(0.0f, 0.0f, 0.0f);
    m_prevGyro = Vector3D(0.0f, 0.0f, 0.0f);
    m_filteredAccel = Vector3D(0.0f, 0.0f, 0.0f);
    m_bufferIndex = 0;
    
    // Reset acceleration buffer
    for (auto& accel : m_accelBuffer) {
        accel = Vector3D(0.0f, 0.0f, 0.0f);
    }
}

Vector3D MotionIsolator::processAcceleration(const Vector3D& rawAccel, const Vector3D& gyro) {
    if (!m_initialized) {
        initialize(100.0f); // Default initialization if not done yet
    }
    
    // Update gravity estimate first
    updateGravityEstimate(rawAccel);
    
    // Calculate linear acceleration by removing gravity and rotational components
    Vector3D linearAccel = isolateLinearAcceleration(rawAccel, gyro);
    
    // Apply high-pass filter to remove drift
    linearAccel = applyHighPassFilter(linearAccel);
    
    // Apply moving average to smooth results
    linearAccel = applyMovingAverage(linearAccel);
    
    // Update previous readings
    m_prevAccel = rawAccel;
    m_prevGyro = gyro;
    
    return linearAccel;
}

void MotionIsolator::setGravityDirection(const Vector3D& gravity) {
    // Set gravity direction manually (useful for calibration)
    float magnitude = gravity.magnitude();
    if (magnitude > 0.0f) {
        // Maintain the standard gravity magnitude (9.81) but use the provided direction
        m_gravity = gravity.normalize() * 9.81f;
    }
}

Vector3D MotionIsolator::applyHighPassFilter(const Vector3D& input) {
    // Simple high-pass filter to remove low frequency components like gravity
    m_filteredAccel = m_filteredAccel * m_alpha + (input - m_prevAccel) * m_beta;
    return m_filteredAccel;
}

Vector3D MotionIsolator::isolateLinearAcceleration(const Vector3D& rawAccel, const Vector3D& gyro) {
    // Remove gravity
    Vector3D accelWithoutGravity = rawAccel - m_gravity;
    
    // Calculate and remove rotational acceleration component
    Vector3D rotationalAccel = calculateRotationalAcceleration(gyro);
    
    // Final linear acceleration
    return accelWithoutGravity - rotationalAccel;
}

Vector3D MotionIsolator::getGravityVector() const {
    return m_gravity;
}

Vector3D MotionIsolator::applyMovingAverage(const Vector3D& newValue) {
    // Add new value to the buffer
    m_accelBuffer[m_bufferIndex] = newValue;
    m_bufferIndex = (m_bufferIndex + 1) % FILTER_WINDOW_SIZE;
    
    // Calculate average
    Vector3D sum(0.0f, 0.0f, 0.0f);
    for (const auto& accel : m_accelBuffer) {
        sum = sum + accel;
    }
    
    return sum * (1.0f / FILTER_WINDOW_SIZE);
}

void MotionIsolator::updateGravityEstimate(const Vector3D& rawAccel) {
    // Complementary filter to estimate gravity direction
    // Uses low-pass filter to extract gravity from acceleration
    m_gravity = m_gravity * m_alpha + rawAccel * (1.0f - m_alpha);
    
    // Ensure gravity has the correct magnitude (approximately 9.81 m/s²)
    float magnitude = m_gravity.magnitude();
    if (magnitude > 0.0f) {
        m_gravity = m_gravity.normalize() * 9.81f;
    }
}

Vector3D MotionIsolator::calculateRotationalAcceleration(const Vector3D& gyro) {
    // Cross product of angular velocity (gyro) and linear velocity
    // This is a simplified model and can be expanded for better accuracy
    
    // For a more accurate model, we would need to:
    // 1. Integrate gyro to get rotation
    // 2. Calculate tangential and centripetal acceleration components
    // 3. Transform the accelerations to the sensor frame
    
    // Simplified version for initial implementation
    float dt = 1.0f / m_sampleRate;
    
    // Estimate change in angular velocity
    Vector3D angularAccel;
    angularAccel.x = (gyro.x - m_prevGyro.x) / dt;
    angularAccel.y = (gyro.y - m_prevGyro.y) / dt;
    angularAccel.z = (gyro.z - m_prevGyro.z) / dt;
    
    // Basic estimation of rotational acceleration effects
    // This is a highly simplified model - for a real implementation,
    // a proper rotational dynamics model would be needed
    Vector3D rotEffect;
    
    // Scale factor to control the influence (may need tuning)
    const float rotationalScale = 0.05f;
    
    rotEffect.x = angularAccel.y * 0.1f + gyro.y * gyro.z * rotationalScale;
    rotEffect.y = angularAccel.x * 0.1f + gyro.x * gyro.z * rotationalScale;
    rotEffect.z = (gyro.x * gyro.x + gyro.y * gyro.y) * rotationalScale;
    
    return rotEffect;
}

} // namespace Isolator 