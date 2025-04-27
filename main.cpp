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
bool g_is_calibrated = false;
const int CALIBRATION_SAMPLES = 50;
int g_calibration_count = 0;
std::unordered_map<std::string, int> g_calibration_data = {
    {"ax", 0}, {"ay", 0}, {"az", 0}, {"gx", 0}, {"gy", 0}, {"gz", 0}
};

// Initialize and connect to the serial port
HANDLE initializeSerialPort(const std::string& portName) {
    HANDLE hSerial = Serial::getSerialHandle(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open serial port." << std::endl;
    } else {
        std::cout << "Reading sensor data. Press Ctrl+C to exit..." << std::endl;
        std::cout << "Press 'R' to recalibrate at any time" << std::endl;
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

// Reset and start recalibration
void resetAndRecalibrate() {
    g_tracker.reset();
    g_is_calibrated = false;
    g_calibration_count = 0;
    g_calibration_data = {
        {"ax", 0}, {"ay", 0}, {"az", 0}, {"gx", 0}, {"gy", 0}, {"gz", 0}
    };
    std::cout << "Starting recalibration..." << std::endl;
    std::cout << "Hold the sensor still..." << std::endl;
}

// Update calibration data
void updateCalibration(const std::unordered_map<std::string, int>& data) {
    g_calibration_data["ax"] += data.at("ax");
    g_calibration_data["ay"] += data.at("ay");
    g_calibration_data["az"] += data.at("az");
    g_calibration_data["gx"] += data.at("gx");
    g_calibration_data["gy"] += data.at("gy");
    g_calibration_data["gz"] += data.at("gz");
    
    g_calibration_count++;
    
    // If we've collected enough samples, perform calibration
    if (g_calibration_count >= CALIBRATION_SAMPLES) {
        // Average the values
        g_calibration_data["ax"] /= CALIBRATION_SAMPLES;
        g_calibration_data["ay"] /= CALIBRATION_SAMPLES;
        g_calibration_data["az"] /= CALIBRATION_SAMPLES;
        g_calibration_data["gx"] /= CALIBRATION_SAMPLES;
        g_calibration_data["gy"] /= CALIBRATION_SAMPLES;
        g_calibration_data["gz"] /= CALIBRATION_SAMPLES;
        
        // Calibrate the tracker
        g_tracker.calibrate(g_calibration_data);
        g_is_calibrated = true;
        
        std::cout << "Calibration complete." << std::endl;
    } else {
        std::cout << "Calibrating... " << g_calibration_count << "/" 
                  << CALIBRATION_SAMPLES << " (hold still)" << std::endl;
    }
}

// Thread function to check for keyboard input
void keyboardThread() {
    while (g_running) {
        if (_kbhit()) {
            int key = _getch();
            
            // Toggle plot visibility with number keys
            if (key >= '1' && key <= '6') {
                // Current state
                bool a_accel = (key == '1');
                bool g_accel = (key == '2');
                bool a_vel = (key == '3');
                bool g_vel = (key == '4');
                bool a_pos = (key == '5');
                bool g_pos = (key == '6');
                
                // Configure plots
                Plot::configurePlots(a_accel, g_accel, a_vel, g_vel, a_pos, g_pos);
                
                std::cout << "Showing plot: ";
                if (a_accel) std::cout << "Accelerometer";
                if (g_accel) std::cout << "Gyroscope";
                if (a_vel) std::cout << "Accel Velocity";
                if (g_vel) std::cout << "Gyro Velocity";
                if (a_pos) std::cout << "Accel Position";
                if (g_pos) std::cout << "Gyro Position";
                std::cout << std::endl;
            }
            
            // 'A' to show all plots
            if (key == 'A' || key == 'a') {
                Plot::configurePlots(true, true, true, true, true, true);
                std::cout << "Showing all plots" << std::endl;
            }
            
            // 'R' or 'r' for reset
            if (key == 'R' || key == 'r') {
                resetAndRecalibrate();
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
    auto last_update_time = std::chrono::high_resolution_clock::now();
    
    // Simple gyro integration for testing
    Hand::Vector3D gyro_velocity = {0, 0, 0};
    Hand::Vector3D gyro_position = {0, 0, 0};
    
    while (g_running) {
        try {
            // Read one complete message
            std::unordered_map<std::string, int> result = readAndProcess(hSerial);
            
            // Print all six IMU values
            stringifyMap(result);
            
            // During initial startup, collect calibration data
            if (!g_is_calibrated) {
                updateCalibration(result);
            } else {
                // Compute time since last update
                auto current_time = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float>(current_time - last_update_time).count();
                last_update_time = current_time;
                
                // Update the hand tracker with new data
                g_tracker.update(result, dt);
                
                // Get accelerometer-derived position and velocity
                Hand::Vector3D accel_position = g_tracker.getPosition();
                Hand::Vector3D accel_velocity = g_tracker.getVelocity();
                
                // Simple gyro integration for testing
                float gx = static_cast<float>(result["gx"]) / 100.0f;
                float gy = static_cast<float>(result["gy"]) / 100.0f;
                float gz = static_cast<float>(result["gz"]) / 100.0f;
                
                // Update gyro velocity with simple integration
                gyro_velocity.x += gx * dt;
                gyro_velocity.y += gy * dt;
                gyro_velocity.z += gz * dt;
                
                // Apply damping to gyro velocity
                float damping = 0.99f;
                gyro_velocity.x *= damping;
                gyro_velocity.y *= damping;
                gyro_velocity.z *= damping;
                
                // Update gyro position with simple integration
                gyro_position.x += gyro_velocity.x * dt;
                gyro_position.y += gyro_velocity.y * dt;
                gyro_position.z += gyro_velocity.z * dt;
                
                // Add data point to the plot with all types of data
                Plot::addDataPoint(
                    result,              // Raw sensor data
                    accel_velocity,      // Accelerometer-derived velocity
                    accel_position,      // Accelerometer-derived position
                    gyro_velocity,       // Gyroscope-derived velocity
                    gyro_position        // Gyroscope-derived position
                );
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
    // Initialize plotting library
    if (!Plot::initialize("Sensor Data Visualization")) {
        std::cerr << "Failed to initialize plotting library" << std::endl;
        return 1;
    }
    
    // Configure which plots to show by default
    Plot::configurePlots(true, false, false, false, false, false);
    
    // Initialize the hand tracker
    g_tracker.reset();
    
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
    std::cout << "1-6: Toggle individual plots" << std::endl;
    std::cout << "  1 - Accelerometer" << std::endl;
    std::cout << "  2 - Gyroscope" << std::endl;
    std::cout << "  3 - Accelerometer Velocity" << std::endl;
    std::cout << "  4 - Gyroscope Velocity" << std::endl;
    std::cout << "  5 - Accelerometer Position" << std::endl;
    std::cout << "  6 - Gyroscope Position" << std::endl;
    std::cout << "A: Show all plots" << std::endl;
    std::cout << "R: Recalibrate sensors" << std::endl;
    std::cout << "ESC: Exit program" << std::endl;
    
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
    
    if (keyboard_thread.joinable()) {
        keyboard_thread.join();
    }
    
    // Clean up
    CloseHandle(hSerial);
    Plot::shutdown();
    
    return 0;
}
