#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "libSerial/serial.h"
#include "libPlot/plot.h"

// Global flag for termination
std::atomic<bool> g_running(true);

// Initialize and connect to the serial port
HANDLE initializeSerialPort(const std::string& portName) {
    HANDLE hSerial = Serial::getSerialHandle(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open serial port." << std::endl;
    } else {
        std::cout << "Reading sensor data. Press Ctrl+C to exit..." << std::endl;
    }
    
    return hSerial;
}

// Display the IMU values
void stringifyMap(std::unordered_map<std::string, int>& result) {
    std::cout << "ax: " << result["ax"] << ", ";
    std::cout << "ay: " << result["ay"] << ", ";
    std::cout << "az: " << result["az"] << ", ";
    std::cout << "gx: " << result["gx"] << ", ";
    std::cout << "gy: " << result["gy"] << ", ";
    std::cout << "gz: " << result["gz"] << std::endl;
}

// Thread function to read sensor data
void sensorThread(HANDLE hSerial) {
    while (g_running) {
        try {
            // Read one complete message
            std::unordered_map<std::string, int> result = readAndProcess(hSerial);
            
            // Print all six IMU values
            stringifyMap(result);
            
            // Add data point to the plot
            Plot::addDataPoint(result);
            
            // Add a small delay between readings
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (const std::exception& e) {
            std::cerr << "Error in sensor thread: " << e.what() << std::endl;
            break;
        }
    }
}

int main() {
    // Initialize plotting library
    if (!Plot::initialize("Motion Capture Visualization")) {
        std::cerr << "Failed to initialize plotting library" << std::endl;
        return 1;
    }
    
    // Connect to the serial port
    std::string portName = "\\\\.\\COM3"; // Adjust as needed (e.g. COM4)
    HANDLE hSerial = initializeSerialPort(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        Plot::shutdown();
        return 1;
    }

    // Start sensor reading thread
    std::thread sensor_thread(sensorThread, hSerial);
    
    // Main rendering loop
    while (Plot::isWindowOpen() && g_running) {
        // Render one frame
        if (!Plot::renderFrame()) {
            break;
        }
    }
    
    // Signal thread to terminate and wait for it
    g_running = false;
    if (sensor_thread.joinable()) {
        sensor_thread.join();
    }
    
    // Clean up
    CloseHandle(hSerial);
    Plot::shutdown();
    
    return 0;
}
