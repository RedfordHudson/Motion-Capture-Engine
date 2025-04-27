#pragma once

#include <vector>
#include <string>
#include <array>
#include <deque>
#include <unordered_map>
#include <mutex>

namespace Plot {
    // Maximum number of points to store in history
    constexpr size_t MAX_POINTS = 100;

    // Data structure to hold time-series data for 6D sensor data
    struct SensorData {
        // Buffers for each dimension
        std::vector<float> ax_data;
        std::vector<float> ay_data;
        std::vector<float> az_data;
        std::vector<float> gx_data;
        std::vector<float> gy_data;
        std::vector<float> gz_data;
        
        // X-axis (time) values
        std::vector<float> times;
        
        // Mutex for thread-safe access
        std::mutex mtx;
    };

    // Initialize the plotting system and create window
    bool initialize(const std::string& title = "Motion Capture Data Visualization");

    // Shutdown the plotting system
    void shutdown();

    // Add a new data point to the plot
    void addDataPoint(const std::unordered_map<std::string, int>& data_point);

    // Draw the plots (call in render loop)
    void drawPlots();

    // Render one frame
    bool renderFrame();

    // Check if the window is open
    bool isWindowOpen();
} 