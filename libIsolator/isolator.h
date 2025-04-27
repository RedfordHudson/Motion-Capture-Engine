#pragma once

#include <vector>
#include <array>
#include <cmath>

namespace Isolator {

// Structure to hold 3D vector data
struct Vector3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Vector3D() = default;
    Vector3D(float x, float y, float z) : x(x), y(y), z(z) {}
    
    // Simple vector operations
    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3D operator*(float scalar) const {
        return Vector3D(x * scalar, y * scalar, z * scalar);
    }
    
    float magnitude() const {
        return std::sqrt(x*x + y*y + z*z);
    }
    
    Vector3D normalize() const {
        float mag = magnitude();
        if (mag > 0.0f) {
            return Vector3D(x / mag, y / mag, z / mag);
        }
        return *this;
    }
};

// Class to isolate linear acceleration from rotational motion and gravity
class MotionIsolator {
public:
    MotionIsolator();
    ~MotionIsolator();
    
    // Initialize the isolator with filter parameters
    void initialize(float sampleRate, float cutoffFreq = 0.5f);
    
    // Process raw sensor data to isolate linear acceleration
    Vector3D processAcceleration(const Vector3D& rawAccel, const Vector3D& gyro);
    
    // Set the gravity direction (can be calibrated when device is at rest)
    void setGravityDirection(const Vector3D& gravity);
    
    // Apply high-pass filter to remove drift
    Vector3D applyHighPassFilter(const Vector3D& input);
    
    // Calculate linear acceleration by removing gravity and rotational components
    Vector3D isolateLinearAcceleration(const Vector3D& rawAccel, const Vector3D& gyro);
    
    // Reset the isolator state
    void reset();
    
    // Get the current estimated gravity vector
    Vector3D getGravityVector() const;

private:
    // Internal state variables
    Vector3D m_gravity;           // Current gravity vector
    Vector3D m_prevAccel;         // Previous acceleration reading
    Vector3D m_prevGyro;          // Previous gyroscope reading
    Vector3D m_filteredAccel;     // Filtered acceleration
    
    // Filter coefficients
    float m_alpha;                // Low-pass filter coefficient for gravity estimation
    float m_beta;                 // High-pass filter coefficient for linear acceleration
    
    // Sample rate and timing
    float m_sampleRate;           // Sample rate in Hz
    float m_lastTime;             // Last update time
    
    // Internal flags
    bool m_initialized;           // Whether the isolator has been initialized
    
    // Buffer for moving average filter
    static constexpr size_t FILTER_WINDOW_SIZE = 10;
    std::array<Vector3D, FILTER_WINDOW_SIZE> m_accelBuffer;
    size_t m_bufferIndex;
    
    // Apply moving average filter
    Vector3D applyMovingAverage(const Vector3D& newValue);
    
    // Estimate gravity from low-frequency components of acceleration
    void updateGravityEstimate(const Vector3D& rawAccel);
    
    // Compute rotational acceleration component
    Vector3D calculateRotationalAcceleration(const Vector3D& gyro);
};

} // namespace Isolator 