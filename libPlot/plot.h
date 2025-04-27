#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "../libHand/hand.h"

namespace Plot {
    // Maximum number of data points to store in history
    constexpr size_t MAX_POINTS = 1000;

    // Structure to hold sensor data for plotting
    struct SensorData {
        std::vector<float> times;
        
        // Accelerometer data (linear acceleration)
        std::vector<float> ax_data;
        std::vector<float> ay_data;
        std::vector<float> az_data;
        
        // Gyroscope data (angular acceleration)
        std::vector<float> gx_data;
        std::vector<float> gy_data;
        std::vector<float> gz_data;
        
        // Accelerometer-derived velocity
        std::vector<float> avx_data;
        std::vector<float> avy_data;
        std::vector<float> avz_data;
        
        // Gyroscope-derived velocity (angular velocity)
        std::vector<float> gvx_data;
        std::vector<float> gvy_data;
        std::vector<float> gvz_data;
        
        // Accelerometer-derived position
        std::vector<float> apx_data;
        std::vector<float> apy_data;
        std::vector<float> apz_data;
        
        // Gyroscope-derived position (angular position)
        std::vector<float> gpx_data;
        std::vector<float> gpy_data;
        std::vector<float> gpz_data;
        
        std::mutex mtx;
    };

    // Initialize plotting system
    bool initialize(const std::string& title);

    // Shutdown plotting system
    void shutdown();

    // Add a data point to the plot
    void addDataPoint(
        const std::unordered_map<std::string, int>& sensor_data,
        const Hand::Vector3D& accel_velocity = {0, 0, 0},
        const Hand::Vector3D& accel_position = {0, 0, 0},
        const Hand::Vector3D& gyro_velocity = {0, 0, 0},
        const Hand::Vector3D& gyro_position = {0, 0, 0}
    );

    // Configure which plots to show
    void configurePlots(bool show_accel, bool show_gyro, bool show_accel_vel, 
                       bool show_gyro_vel, bool show_accel_pos, bool show_gyro_pos);

    // Render one frame
    bool renderFrame();

    // Check if window is still open
    bool isWindowOpen();
} 