#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <conio.h> // For _kbhit() and _getch()
#include <filesystem>
#include "libSerial/serial.h"
#include "libPlot/plot.h"
#include "libHand/hand.h"
#include "libAudio/Audio.h"
#include "libCalibrator/calibration.h"

// Global flag for termination
std::atomic<bool> g_running(true);

// Global HandTracker
Hand::HandTracker g_tracker;

// Global Calibrator
Calibration::Calibrator g_calibrator;

// Path to the calibration sound
std::string g_calibrationSoundPath;

// Function to initialize audio
bool initializeAudio() {
    // Try to use system sounds first - will always succeed
    g_calibrationSoundPath = "SystemDefault";
    std::cout << "Using system sounds for audio feedback" << std::endl;
    return true;
}

// Function to play the calibration sound
void playCalibrationSound() {
    // Use Windows system sounds instead of files
    // SND_ALIAS uses predefined system sounds
    BOOL result = PlaySoundA("SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);
    
    if (!result) {
        // Fallback to simple beep if system sound failed
        std::cerr << "Falling back to system beep" << std::endl;
        Beep(750, 300); // 750 Hz for 300 ms
    }
}

// Function to handle calibration complete
void onCalibrationComplete(const Calibration::CalibrationResults& results) {
    // Play a different sound when calibration completes
    PlaySoundA("SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
    
    // Additional actions when calibration completes
    std::cout << "Calibration offsets can be applied to sensor data" << std::endl;
}

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
    // Comment out the sensor data logging as requested
    /*
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
    */
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
            // Start calibration with 'C' key
            else if (key == 'c' || key == 'C') {
                playCalibrationSound();
                g_calibrator.startCalibration(5, nullptr, onCalibrationComplete);
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
            
            // Print all six IMU values (now commented out)
            stringifyMap(result);
            
            // Update the hand tracker with new data
            g_tracker.update(result);
            
            // Add data point to the plot
            Plot::addDataPoint(result);
            
            // Update calibration state if active
            if (g_calibrator.isCalibrating()) {
                g_calibrator.update(&result);
            } else {
                g_calibrator.update(nullptr);
            }
            
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
    // Initialize audio
    bool audioInitialized = initializeAudio();
    if (!audioInitialized) {
        std::cerr << "Warning: Audio initialization failed. Continuing without audio." << std::endl;
    }
    
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
    std::cout << "C: Start calibration" << std::endl;
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
    
    // Clean up
    ::CloseHandle(hSerial);
    Plot::shutdown();
    
    return 0;
}
