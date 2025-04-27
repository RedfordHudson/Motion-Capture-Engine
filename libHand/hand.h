#pragma once

#include <array>
#include <unordered_map>
#include <vector>
#include <string>

namespace Hand {
    // Represents a 3D vector for position, velocity, or acceleration
    struct Vector3D {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        
        // Initialize with zeros
        Vector3D() = default;
        
        // Initialize with values
        Vector3D(float x_val, float y_val, float z_val) 
            : x(x_val), y(y_val), z(z_val) {}
        
        // Vector addition
        Vector3D operator+(const Vector3D& other) const {
            return Vector3D(x + other.x, y + other.y, z + other.z);
        }
        
        // Vector scaling
        Vector3D operator*(float scalar) const {
            return Vector3D(x * scalar, y * scalar, z * scalar);
        }
    };

    // Class to track hand position based on accelerometer data
    class HandTracker {
    public:
        // Constructor
        HandTracker();
        
        // Reset the tracker to initial position and velocity
        void reset();
        
        // Set the sensor calibration values (measured at rest)
        void calibrate(const std::unordered_map<std::string, int>& calibration_data);
        
        // Update position using new accelerometer data
        // dt is the time step in seconds
        void update(const std::unordered_map<std::string, int>& sensor_data, float dt);
        
        // Get the current position
        Vector3D getPosition() const;
        
        // Get the current velocity
        Vector3D getVelocity() const;
        
        // Get the raw acceleration (after calibration)
        Vector3D getAcceleration() const;
        
        // Get history of positions for visualization
        const std::vector<Vector3D>& getPositionHistory() const;
        
    private:
        // Current state
        Vector3D position_;
        Vector3D velocity_;
        Vector3D acceleration_;
        
        // Calibration offsets (measured at rest)
        Vector3D accel_offset_;
        
        // Constants
        static constexpr float kGravity = 9.81f;  // m/s^2
        static constexpr float kAccelScale = 1.0f / 16384.0f;  // Convert raw to g (for typical 16-bit sensor)
        
        // High-pass filter parameter for drift correction
        float alpha_ = 0.7f;
        
        // Position history for visualization
        std::vector<Vector3D> position_history_;
        static constexpr size_t kMaxHistorySize = 100;
        
        // Apply a simple high-pass filter to remove drift
        Vector3D applyHighPassFilter(const Vector3D& prev_filtered, 
                                    const Vector3D& current, 
                                    const Vector3D& prev_raw);
    };
} 