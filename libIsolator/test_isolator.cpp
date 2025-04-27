#include "isolator.h"
#include <iostream>
#include <iomanip>
#include <vector>

// Simple function to print vector values
void printVector(const char* label, const Isolator::Vector3D& vec) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << label << ": [" 
              << std::setw(7) << vec.x << ", " 
              << std::setw(7) << vec.y << ", " 
              << std::setw(7) << vec.z << "]" << std::endl;
}

int main() {
    std::cout << "Motion Capture Engine - Isolator Test\n";
    std::cout << "=====================================\n\n";
    
    // Create instance of motion isolator
    Isolator::MotionIsolator isolator;
    
    // Initialize with sample rate and cutoff frequency
    isolator.initialize(100.0f, 0.5f); // 100Hz sample rate, 0.5Hz cutoff
    
    // Test data: simulated acceleration readings
    // Each vector represents [x, y, z] acceleration in m/s²
    std::vector<Isolator::Vector3D> rawAccelSamples = {
        {0.0f, 0.0f, 9.81f},      // At rest, only gravity
        {0.0f, 0.0f, 9.81f},
        {1.0f, 0.0f, 9.81f},      // Acceleration in x direction
        {2.0f, 0.0f, 9.81f},
        {3.0f, 0.0f, 9.81f},
        {2.0f, 0.0f, 9.81f},      // Slowing down
        {1.0f, 0.0f, 9.81f},
        {0.0f, 0.0f, 9.81f},      // Back to rest
        {0.0f, 1.0f, 9.81f},      // Acceleration in y direction
        {0.0f, 2.0f, 9.81f},
        {0.0f, 1.0f, 9.81f},
        {0.0f, 0.0f, 9.81f},      // Back to rest
        {0.0f, 0.0f, 10.81f},     // Acceleration in z direction
        {0.0f, 0.0f, 11.81f},
        {0.0f, 0.0f, 10.81f},
        {0.0f, 0.0f, 9.81f}       // Back to rest
    };
    
    // Simulated gyroscope readings (angular velocity in rad/s)
    std::vector<Isolator::Vector3D> gyroSamples(rawAccelSamples.size(), {0.0f, 0.0f, 0.0f});
    
    // Now add some rotation to demonstrate how the isolator handles it
    gyroSamples[8]  = {0.0f, 0.0f, 0.5f};  // Rotation around z-axis
    gyroSamples[9]  = {0.0f, 0.0f, 1.0f};
    gyroSamples[10] = {0.0f, 0.0f, 0.5f};
    gyroSamples[11] = {0.0f, 0.0f, 0.0f};
    
    // Process the data
    std::cout << "Processing simulated sensor data...\n\n";
    
    for (size_t i = 0; i < rawAccelSamples.size(); ++i) {
        std::cout << "Sample " << i + 1 << ":\n";
        
        // Print raw input data
        printVector("Raw Accel", rawAccelSamples[i]);
        printVector("Gyro     ", gyroSamples[i]);
        
        // Process data through the isolator
        Isolator::Vector3D linearAccel = isolator.processAcceleration(rawAccelSamples[i], gyroSamples[i]);
        
        // Print the results
        printVector("Linear   ", linearAccel);
        printVector("Gravity  ", isolator.getGravityVector());
        
        std::cout << std::endl;
    }
    
    std::cout << "Test complete.\n";
    return 0;
} 