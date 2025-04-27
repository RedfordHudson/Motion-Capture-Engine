#pragma once

#include <unordered_map>
#include <chrono>

namespace Hand {
    // Simple 3D vector structure
    struct Vector3D {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        
        // Default constructor initializes to zero
        Vector3D() = default;
        
        // Constructor with values
        Vector3D(float x_val, float y_val, float z_val) 
            : x(x_val), y(y_val), z(z_val) {}
    };

    // Extremely simple Hand data class that only stores raw data
    class HandTracker {
    public:
        // Constructor
        HandTracker();
        
        // Update with new sensor data (no processing)
        void update(const std::unordered_map<std::string, int>& data);
        
        // Get raw acceleration
        Vector3D getAcceleration() const;
        
        // Get raw gyroscope
        Vector3D getGyroscope() const;
        
        // Get calculated velocity
        Vector3D getVelocity() const;
        
        // Set calibration offsets
        void setCalibrationOffsets(float ax_offset, float ay_offset, float az_offset,
                                  float gx_offset, float gy_offset, float gz_offset);
        
        // Apply calibration to value
        int applyCalibratedOffset(int value, float offset) const;
        
        // Enable/disable calibration
        void enableCalibration(bool enable);
        
        // Check if calibration is enabled
        bool isCalibrationEnabled() const;
        
    private:
        // Update velocity based on acceleration data
        void updateVelocity(const Vector3D& prevAccel, const std::chrono::steady_clock::time_point& currentTime);
        
        // Raw sensor data
        Vector3D m_accel;
        Vector3D m_gyro;
        
        // Calculated velocity
        Vector3D m_velocity;
        
        // Timestamp for velocity calculation
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        bool m_firstUpdate;
        
        // Calibration offsets
        Vector3D m_accelOffset;
        Vector3D m_gyroOffset;
        
        // Calibration state
        bool m_calibrationEnabled;
    };
} 