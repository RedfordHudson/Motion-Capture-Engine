#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

// Forward declaration
namespace Hand {
    struct Vector3D;
    class HandTracker;
}

namespace Plot {
    // Maximum number of data points to store
    constexpr int MAX_POINTS = 1000;
    
    // Data structure to store sensor data for plotting
    struct SensorData {
        std::vector<float> times;
        
        // Accelerometer data
        std::vector<float> ax_data;
        std::vector<float> ay_data;
        std::vector<float> az_data;
        
        // Gyroscope data
        std::vector<float> gx_data;
        std::vector<float> gy_data;
        std::vector<float> gz_data;
        
        // Velocity data
        std::vector<float> vx_data;
        std::vector<float> vy_data;
        std::vector<float> vz_data;
        
        // Mutex for thread-safe access
        std::mutex mtx;
    };
    
    // Initialize plotting system
    bool initialize(const std::string& title = "Sensor Data Visualization");
    
    // Shutdown plotting system
    void shutdown();
    
    // Configure which plots to show
    void configurePlots(bool show_accelerometer, bool show_gyroscope, bool show_velocity = false);
    
    // Add data point for plotting
    void addDataPoint(const std::unordered_map<std::string, int>& sensor_data);
    
    // Render a new frame (call this in your main loop)
    bool renderFrame();
    
    // Check if the window is still open
    bool isWindowOpen();
} 