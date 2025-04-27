#include "hand.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Hand {
    HandTracker::HandTracker() {
        reset();
    }

    void HandTracker::reset() {
        // Reset all state variables
        position_ = Vector3D();
        velocity_ = Vector3D();
        acceleration_ = Vector3D();
        accel_offset_ = Vector3D();
        
        // Clear history
        position_history_.clear();
        
        // Add initial position to history
        position_history_.push_back(position_);
    }

    void HandTracker::calibrate(const std::unordered_map<std::string, int>& calibration_data) {
        try {
            // Store the accelerometer values at rest as offsets
            accel_offset_.x = static_cast<float>(calibration_data.at("ax"));
            accel_offset_.y = static_cast<float>(calibration_data.at("ay"));
            accel_offset_.z = static_cast<float>(calibration_data.at("az"));
            
            // Note: we don't subtract 1g from z anymore - we take the full calibration reading
            // This way whatever orientation the sensor is in, we properly zero it
            
            std::cout << "Calibration complete. Offsets: ("
                      << accel_offset_.x << ", "
                      << accel_offset_.y << ", "
                      << accel_offset_.z << ")" << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Error during calibration: " << e.what() << std::endl;
        }
    }

    void HandTracker::update(const std::unordered_map<std::string, int>& sensor_data, float dt) {
        try {
            // Limit dt to prevent huge jumps if there's a pause
            dt = std::min(dt, 0.1f);
            
            // Store previous raw acceleration for filtering
            Vector3D prev_accel = acceleration_;
            Vector3D prev_velocity = velocity_;
            
            // Get new accelerometer readings and apply calibration offset
            Vector3D raw_accel;
            raw_accel.x = static_cast<float>(sensor_data.at("ax")) - accel_offset_.x;
            raw_accel.y = static_cast<float>(sensor_data.at("ay")) - accel_offset_.y;
            raw_accel.z = static_cast<float>(sensor_data.at("az")) - accel_offset_.z;
            
            // Convert to m/s² (assuming raw values are in 16-bit range for +/-2g)
            raw_accel.x *= kAccelScale * kGravity;
            raw_accel.y *= kAccelScale * kGravity;
            raw_accel.z *= kAccelScale * kGravity;
            
            // Apply noise threshold to raw acceleration
            const float accel_threshold = 0.05f; // m/s²
            if (std::abs(raw_accel.x) < accel_threshold) raw_accel.x = 0;
            if (std::abs(raw_accel.y) < accel_threshold) raw_accel.y = 0;
            if (std::abs(raw_accel.z) < accel_threshold) raw_accel.z = 0;
            
            // Apply high-pass filter to remove drift
            acceleration_ = applyHighPassFilter(prev_accel, raw_accel, prev_accel);
            
            // Debug output of the raw values vs filtered
            // std::cout << "Raw: (" << raw_accel.x << ", " << raw_accel.y << ", " << raw_accel.z << ") ";
            // std::cout << "Filtered: (" << acceleration_.x << ", " << acceleration_.y << ", " << acceleration_.z << ")\n";
            
            // Improved Euler integration for velocity (average of current and previous acceleration)
            Vector3D avg_accel;
            avg_accel.x = (acceleration_.x + prev_accel.x) * 0.5f;
            avg_accel.y = (acceleration_.y + prev_accel.y) * 0.5f;
            avg_accel.z = (acceleration_.z + prev_accel.z) * 0.5f;
            
            velocity_.x += avg_accel.x * dt;
            velocity_.y += avg_accel.y * dt;
            velocity_.z += avg_accel.z * dt;
            
            // Apply velocity threshold to reduce drift (increased threshold)
            const float velocity_threshold = 0.02f; // m/s - increased to be more aggressive
            if (std::abs(velocity_.x) < velocity_threshold) velocity_.x = 0;
            if (std::abs(velocity_.y) < velocity_threshold) velocity_.y = 0;
            if (std::abs(velocity_.z) < velocity_threshold) velocity_.z = 0;
            
            // Apply velocity damping factor to prevent endless drift
            const float damping = 0.98f; // Slight damping to gradually reduce velocity
            velocity_.x *= damping;
            velocity_.y *= damping;
            velocity_.z *= damping;
            
            // Improved Euler integration for position (average of current and previous velocity)
            Vector3D avg_vel;
            avg_vel.x = (velocity_.x + prev_velocity.x) * 0.5f;
            avg_vel.y = (velocity_.y + prev_velocity.y) * 0.5f;
            avg_vel.z = (velocity_.z + prev_velocity.z) * 0.5f;
            
            position_.x += avg_vel.x * dt;
            position_.y += avg_vel.y * dt;
            position_.z += avg_vel.z * dt;
            
            // Add to position history
            position_history_.push_back(position_);
            
            // Limit history size
            if (position_history_.size() > kMaxHistorySize) {
                position_history_.erase(position_history_.begin());
            }
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Error during update: " << e.what() << std::endl;
        }
    }

    Vector3D HandTracker::getPosition() const {
        return position_;
    }

    Vector3D HandTracker::getVelocity() const {
        return velocity_;
    }

    Vector3D HandTracker::getAcceleration() const {
        return acceleration_;
    }

    const std::vector<Vector3D>& HandTracker::getPositionHistory() const {
        return position_history_;
    }

    Vector3D HandTracker::applyHighPassFilter(const Vector3D& prev_filtered, 
                                             const Vector3D& current, 
                                             const Vector3D& prev_raw) {
        // Simple high-pass filter: y[n] = α * (y[n-1] + x[n] - x[n-1])
        // This helps remove constant acceleration (like gravity) and reduces drift
        
        // Use a stronger high-pass filter (lower alpha) to remove more drift
        // 0.8 -> 0.7
        Vector3D result;
        result.x = alpha_ * (prev_filtered.x + current.x - prev_raw.x);
        result.y = alpha_ * (prev_filtered.y + current.y - prev_raw.y);
        result.z = alpha_ * (prev_filtered.z + current.z - prev_raw.z);
        
        return result;
    }
} 