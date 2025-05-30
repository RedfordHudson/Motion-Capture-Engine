#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <conio.h> // For _kbhit() and _getch()
#include <filesystem>
#include <algorithm> // For std::min and std::max
#include "libSerial/serial.h"
#include "libPlot/plot.h"
#include "libHand/hand.h"
#include "libAudio/Audio.h"
#include "libCalibrator/calibration.h"
#include "libUncoupler/uncoupler.h"

// Global flag for termination
std::atomic<bool> g_running(true);

// Global HandTracker
Hand::HandTracker g_tracker;

// Global Calibrator
Calibration::Calibrator g_calibrator;

// Global Uncoupler
Uncoupler::SensorUncoupler g_uncoupler;

// Path to the calibration sound
std::string g_calibrationSoundPath;
bool g_audioInitialized = false;

// Function to initialize audio
bool initializeAudio() {
    // Get the current executable path
    std::filesystem::path exePath = std::filesystem::current_path();
    
    // Try different locations for the WAV file
    std::vector<std::filesystem::path> possiblePaths = {
        exePath / "assets" / "calibrating.wav",
        exePath.parent_path() / "assets" / "calibrating.wav"
    };
    
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            g_calibrationSoundPath = path.string();
            std::cout << "Found calibration WAV file at: " << g_calibrationSoundPath << std::endl;
            
            // Don't play the sound during initialization, just check if it exists
            g_audioInitialized = true;
            return true;
        }
    }
    
    std::cerr << "Could not find a usable calibration sound file. Falling back to system sounds." << std::endl;
    g_audioInitialized = false;
    return false;
}

// Function to play the calibration sound
void playCalibrationSound() {
    if (g_audioInitialized && !g_calibrationSoundPath.empty()) {
        // Play the WAV file
        if (!Audio::playSoundSimple(g_calibrationSoundPath, true)) {
            // Fall back to system sounds if WAV file fails
            std::cerr << "Failed to play WAV file, falling back to system sounds" << std::endl;
            PlaySoundA("SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);
        }
    } else {
        // Fallback to system sounds
        PlaySoundA("SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);
        
        if (GetLastError() != 0) {
            // Last resort: use Beep
            Beep(750, 300); // 750 Hz for 300 ms
        }
    }
}

// Function to handle calibration complete
void onCalibrationComplete(const Calibration::CalibrationResults& results) {
    // Use system sound when calibration completes
    PlaySoundA("SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
    
    // Apply calibration offsets to the hand tracker
    g_tracker.setCalibrationOffsets(
        static_cast<float>(results.ax_avg),
        static_cast<float>(results.ay_avg),
        static_cast<float>(results.az_avg),
        static_cast<float>(results.gx_avg),
        static_cast<float>(results.gy_avg),
        static_cast<float>(results.gz_avg)
    );
    
    // Also set the gyroscope calibration offsets for the uncoupler
    g_uncoupler.setGyroCalibrationOffsets(
        static_cast<float>(results.gx_avg),
        static_cast<float>(results.gy_avg),
        static_cast<float>(results.gz_avg)
    );
    
    // Enable calibration by default after completion
    g_tracker.enableCalibration(true);
    g_uncoupler.enableGyroCalibration(true);
    
    // Additional actions when calibration completes
    std::cout << "Calibration offsets applied to sensor data" << std::endl;
    std::cout << "Calibration values:" << std::endl;
    std::cout << "  Accel: X=" << results.ax_avg << ", Y=" << results.ay_avg << ", Z=" << results.az_avg << std::endl;
    std::cout << "  Gyro: X=" << results.gx_avg << ", Y=" << results.gy_avg << ", Z=" << results.gz_avg << std::endl;
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
                Plot::configurePlots(true, false, false, false, false);
                std::cout << "Showing accelerometer plot only" << std::endl;
            }
            else if (key == '2') {
                Plot::configurePlots(false, true, false, false, false);
                std::cout << "Showing gyroscope plot only" << std::endl;
            }
            else if (key == '3') {
                Plot::configurePlots(true, true, false, false, false);
                std::cout << "Showing accelerometer and gyroscope plots" << std::endl;
            }
            else if (key == '4') {
                Plot::configurePlots(true, true, true, false, false);
                std::cout << "Showing accelerometer, gyroscope, and velocity plots" << std::endl;
            }
            else if (key == '5') {
                Plot::configurePlots(false, false, false, true, false);
                std::cout << "Showing gravity vector plot" << std::endl;
            }
            else if (key == '6') {
                Plot::configurePlots(false, false, false, false, true);
                std::cout << "Showing linear acceleration plot" << std::endl;
            }
            else if (key == '7') {
                Plot::configurePlots(true, true, true, true, true);
                std::cout << "Showing all plots" << std::endl;
            }
            // Start calibration with 'C' key
            else if (key == 'c' || key == 'C') {
                playCalibrationSound();
                g_calibrator.startCalibration(5, nullptr, onCalibrationComplete);
            }
            // Toggle calibration with 'T' key
            else if (key == 't' || key == 'T') {
                bool newState = !g_tracker.isCalibrationEnabled();
                g_tracker.enableCalibration(newState);
                g_uncoupler.enableGyroCalibration(newState);
                std::cout << "Calibration " << (newState ? "enabled" : "disabled") << std::endl;
            }
            // Reset calibration with 'R' key
            else if (key == 'r' || key == 'R') {
                g_tracker.setCalibrationOffsets(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                g_uncoupler.setGyroCalibrationOffsets(0.0f, 0.0f, 0.0f);
                std::cout << "Calibration reset to zero" << std::endl;
            }
            // Increase gravity filter smoothing with 'S' key
            else if (key == 's' || key == 'S') {
                static float currentAlpha = 0.02f;  // Default alpha
                currentAlpha = (std::max)(0.001f, currentAlpha * 0.8f);  // Reduce by 20%
                g_uncoupler.setLowPassFilterAlpha(currentAlpha);
                std::cout << "Increased gravity smoothing (alpha = " << currentAlpha << ")" << std::endl;
            }
            // Decrease gravity filter smoothing with 'F' key
            else if (key == 'f' || key == 'F') {
                static float currentAlpha = 0.02f;  // Default alpha
                currentAlpha = (std::min)(0.5f, currentAlpha * 1.25f);  // Increase by 25%
                g_uncoupler.setLowPassFilterAlpha(currentAlpha);
                std::cout << "Decreased gravity smoothing (alpha = " << currentAlpha << ")" << std::endl;
            }
            // Increase gravity filter window size with '+' key
            else if (key == 43 || key == 61) {  // 43 is '+', 61 is '='
                static size_t currentWindowSize = 50;  // Default window size
                currentWindowSize = (std::min)(static_cast<size_t>(500), currentWindowSize + 10);
                g_uncoupler.setGravityFilterSize(currentWindowSize);
                std::cout << "Increased gravity filter window size to " << currentWindowSize << " samples" << std::endl;
            }
            // Decrease gravity filter window size with '-' key
            else if (key == 45 || key == 95) {  // 45 is '-', 95 is '_'
                static size_t currentWindowSize = 50;  // Default window size
                currentWindowSize = (std::max)(static_cast<size_t>(10), currentWindowSize - 10);
                g_uncoupler.setGravityFilterSize(currentWindowSize);
                std::cout << "Decreased gravity filter window size to " << currentWindowSize << " samples" << std::endl;
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
            
            // Process raw data through the uncoupler to get gravity vector estimation
            Uncoupler::UncoupledData uncoupledData = g_uncoupler.processData(result);
            
            // Update the hand tracker with raw data
            g_tracker.update(result);

            
            // Use calibrated values from the hand tracker instead of raw data
            std::unordered_map<std::string, int> calibratedData;
            Hand::Vector3D accel = g_tracker.getAcceleration();
            Hand::Vector3D gyro = g_tracker.getGyroscope();
            
            calibratedData["ax"] = static_cast<int>(accel.x);
            calibratedData["ay"] = static_cast<int>(accel.y);
            calibratedData["az"] = static_cast<int>(accel.z);
            calibratedData["gx"] = static_cast<int>(gyro.x);
            calibratedData["gy"] = static_cast<int>(gyro.y);
            calibratedData["gz"] = static_cast<int>(gyro.z);
            
            // Add calibrated data point to the plot
            Plot::addDataPoint(calibratedData);
            
            // Add data to plot with gravity and linear acceleration information
            Plot::addDataPointWithGravity(
                calibratedData,
                uncoupledData.grav_x,
                uncoupledData.grav_y, 
                uncoupledData.grav_z,
                uncoupledData.ax_linear,
                uncoupledData.ay_linear,
                uncoupledData.az_linear
            );
            
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
        std::cerr << "Warning: Audio WAV file initialization failed. Will use system sounds instead." << std::endl;
    }
    
    // Initialize plotting library
    if (!Plot::initialize("Motion Capture Data Visualization")) {
        std::cerr << "Failed to initialize plotting library" << std::endl;
        return 1;
    }
    
    // Configure which plots to show by default
    Plot::configurePlots(true, true, false, true, false);
    
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
    std::cout << "4: Show accelerometer, gyroscope, and velocity plots" << std::endl;
    std::cout << "5: Show gravity vector plot" << std::endl;
    std::cout << "6: Show linear acceleration plot" << std::endl;
    std::cout << "7: Show all plots" << std::endl;
    std::cout << "C: Start calibration" << std::endl;
    std::cout << "T: Toggle calibration on/off" << std::endl;
    std::cout << "R: Reset calibration" << std::endl;
    std::cout << "S: Increase gravity smoothing" << std::endl;
    std::cout << "F: Decrease gravity smoothing" << std::endl;
    std::cout << "+: Increase gravity filter window size" << std::endl;
    std::cout << "-: Decrease gravity filter window size" << std::endl;
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
    CloseHandle(hSerial);
    Plot::shutdown();
    
    return 0;
}
