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
        std::vector<float> ax_data;
        std::vector<float> ay_data;
        std::vector<float> az_data;
        std::mutex mtx;
    };

    // Initialize plotting system
    bool initialize(const std::string& title);

    // Shutdown plotting system
    void shutdown();

    // Add a data point to the plot
    void addDataPoint(const std::unordered_map<std::string, int>& data_point);

    // Render one frame
    bool renderFrame();

    // Check if window is still open
    bool isWindowOpen();
} 