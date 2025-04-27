#include <iostream>
#include <thread>
#include <chrono>
#include "libSerial/serial.h"

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

// Read and print sensor data
void processSensorData(HANDLE hSerial) {
    // Read one complete message
    std::unordered_map<std::string, int> result = readAndProcess(hSerial);
    
    // Print all six IMU values
    stringifyMap(result);
}

// Main loop to continuously read and process data
void captureLoop(HANDLE hSerial) {
    while (true) {
        processSensorData(hSerial);
        
        // Add a small delay between readings
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    // Connect to the serial port
    std::string portName = "\\\\.\\COM3"; // Adjust as needed (e.g. COM4)
    HANDLE hSerial = initializeSerialPort(portName);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        return 1;
    }

    // Continuously read and print sensor data
    try {
        captureLoop(hSerial);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Close the serial port
    CloseHandle(hSerial);
    return 0;
}
