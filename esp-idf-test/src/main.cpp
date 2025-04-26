#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"

// Variables to store sensor data
int flexValue1, flexValue2;
float ax, ay, az, gx, gy, gz;

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  // Wait for Serial Monitor to open
  delay(1000);
  Serial.println("Multi-Sensor Reading Started");
  
  // Initialize I2C for MPU6050
  Wire.begin();
  
  // Initialize sensors
  setupFlexSensors();
  bool mpuSuccess = setupMPU6050();
  
  if (!mpuSuccess) {
    Serial.println("Warning: MPU6050 initialization failed. Continuing with flex sensors only.");
  }
}

void loop() {
  // Read data from all sensors
  readFlexSensors(flexValue1, flexValue2);
  
  // Try to read MPU6050 data (will only work if initialization was successful)
  readMPU6050(ax, ay, az, gx, gy, gz);
  
  // Print all sensor data
  Serial.println("-----------------------------------");
  printFlexSensorData(flexValue1, flexValue2);
  printMPU6050Data(ax, ay, az, gx, gy, gz);
  Serial.println("-----------------------------------");
  
  // Small delay
  delay(200);
}
