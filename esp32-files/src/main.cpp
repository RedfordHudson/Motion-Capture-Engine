#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"

// Mode selection (true = scan for I2C devices, false = read sensor data)
bool scanMode = false;

// Variables to store sensor data
int flexValue1 = 0, flexValue2 = 0;
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
bool mpuInitialized = false;

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// Timing parameters
#define SAMPLE_RATE_HZ 50  // 50Hz = 20ms between readings

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  delay(1000);  // Give time for serial monitor to open
  
  Serial.println("==== Multi-Sensor System ====");
  Serial.println("Current mode: " + String(scanMode ? "I2C SCANNER" : "SENSOR READING"));
  Serial.println("Sample rate: " + String(SAMPLE_RATE_HZ) + "Hz");
  Serial.println("Output format: JSON");
  
  // Initialize I2C bus
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Setup sensors if in reading mode
  if (!scanMode) {
    // Setup flex sensors
    setupFlexSensor();
    
    // Setup MPU6050
    mpuInitialized = setupMPU6050();
    if (!mpuInitialized) {
      Serial.println("Warning: Will continue with flex sensors only");
    }
  }
}

void loop() {
  if (scanMode) {
    // Run I2C scanner
    scanI2C();
  } else {
    // Read from flex sensors
    flexValue1 = readFlexSensor1();
    flexValue2 = readFlexSensor2();
    
    // Read from MPU6050 if available
    if (mpuInitialized) {
      readMPU6050(ax, ay, az, gx, gy, gz);
    }
    
    // Output in JSON format
    Serial.print("{");
    
    // Flex sensor data
    Serial.print("\"flex1\":");
    Serial.print(flexValue1);
    Serial.print(",\"flex2\":");
    Serial.print(flexValue2);
    
    // MPU6050 data
    if (mpuInitialized) {
      Serial.print(",\"accel\":{\"x\":");
      Serial.print(ax, 2);
      Serial.print(",\"y\":");
      Serial.print(ay, 2);
      Serial.print(",\"z\":");
      Serial.print(az, 2);
      Serial.print("},\"gyro\":{\"x\":");
      Serial.print(gx, 2);
      Serial.print(",\"y\":");
      Serial.print(gy, 2);
      Serial.print(",\"z\":");
      Serial.print(gz, 2);
      Serial.print("}");
    } else {
      // Empty objects if MPU not available
      Serial.print(",\"accel\":{\"x\":0,\"y\":0,\"z\":0},\"gyro\":{\"x\":0,\"y\":0,\"z\":0}");
    }
    
    // Close JSON object and end line
    Serial.println("}");
    
    // Delay between readings - faster rate
    delay(1000 / SAMPLE_RATE_HZ); // Convert Hz to milliseconds
  }
}