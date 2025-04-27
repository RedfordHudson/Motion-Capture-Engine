#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <conio.h> // For _kbhit() and _getch()
#include "libSerial/serial.h"
#include "libPlot/plot.h"
#include "libHand/hand.h"

// Global flag for termination
std::atomic<bool> g_running(true);

// Global HandTracker
Hand::HandTracker g_tracker;

// Initialize and connect to the serial port
HANDLE initializeSerialPort(const std::string& portName) {
    HANDLE hSerial = Serial::getSerialHandle(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open serial port." << std::endl;
    } else {
        std::cout << "Reading sensor data. Press ESC to exit..." << std::endl;
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
    std::cout << "gz: " << result["gz"] << ", ";
    
    // Display calculated velocity
    Hand::Vector3D velocity = g_tracker.getVelocity();
    std::cout << "vx: " << velocity.x << ", ";
    std::cout << "vy: " << velocity.y << ", ";
    std::cout << "vz: " << velocity.z << std::endl;
}

// Thread function to check for keyboard input
void keyboardThread() {
    while (g_running) {
        if (_kbhit()) {
            int key = _getch();
            
            // Toggle plot visibility with number keys
            if (key == '1') {
                Plot::configurePlots(true, false, false);
                std::cout << "Showing accelerometer plot only" << std::endl;
            }
            else if (key == '2') {
                Plot::configurePlots(false, true, false);
                std::cout << "Showing gyroscope plot only" << std::endl;
            }
            else if (key == '3') {
                Plot::configurePlots(true, true, false);
                std::cout << "Showing accelerometer and gyroscope plots" << std::endl;
            }
            else if (key == '4') {
                Plot::configurePlots(true, true, true);
                std::cout << "Showing all plots (accelerometer, gyroscope, and velocity)" << std::endl;
            }
            
            // ESC key to exit
            if (key == 27) {
                g_running = false;
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Thread function to read sensor data
void sensorThread(HANDLE hSerial) {
    while (g_running) {
        try {
            // Read one complete message using the global function (not in Serial namespace)
            std::unordered_map<std::string, int> result = ::readAndProcess(hSerial);
            
            // Print all six IMU values
            stringifyMap(result);
            
            // Update the hand tracker with new data
            g_tracker.update(result);
            
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
    if (!Plot::initialize("Raw Sensor Data Visualization")) {
        std::cerr << "Failed to initialize plotting library" << std::endl;
        return 1;
    }
    
    // Configure which plots to show by default
    Plot::configurePlots(true, true);
    
    // Connect to the serial port
    std::string portName = "\\\\.\\COM3"; // Adjust as needed (e.g. COM4)
    HANDLE hSerial = initializeSerialPort(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        Plot::shutdown();
        return 1;
    }

    // Start sensor reading thread
    std::thread sensor_thread(sensorThread, hSerial);
    
    // Start keyboard input thread
    std::thread keyboard_thread(keyboardThread);
    
    // Print instructions
    std::cout << "Keyboard Controls:" << std::endl;
    std::cout << "1: Show accelerometer plot only" << std::endl;
    std::cout << "2: Show gyroscope plot only" << std::endl;
    std::cout << "3: Show accelerometer and gyroscope plots" << std::endl;
    std::cout << "4: Show all plots (accelerometer, gyroscope, and velocity)" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    
    // Main rendering loop
    while (g_running && Plot::isWindowOpen()) {
        // Render frame and break loop if window is closed
        if (!Plot::renderFrame()) {
            g_running = false;
            break;
        }
    }
    
    // Wait for threads to finish
    g_running = false;
    if (sensor_thread.joinable()) {
        sensor_thread.join();
    }
    if (keyboard_thread.joinable()) {
        keyboard_thread.join();
    }
    
    // Clean up - CloseHandle is a Windows API function
    ::CloseHandle(hSerial);
    Plot::shutdown();
    
    return 0;
}
