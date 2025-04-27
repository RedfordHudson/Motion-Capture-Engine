#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"

// MPU6050 constants
#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_GYRO_START   0x3B  // Gyroscope data comes first
#define MPU6050_ACCEL_START  0x43  // Accelerometer data comes second

// Initialize MPU6050
bool setupMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1);
  Wire.write(0); // Wake up MPU6050
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("MPU6050 initialized successfully");
    return true;
  } else {
    Serial.print("MPU6050 initialization failed, error code: ");
    Serial.println(error);
    return false;
  }
}

// Read MPU6050 data
void readMPU6050(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_GYRO_START);
  if (Wire.endTransmission(false) != 0) {
    return;
  }
  
  Wire.requestFrom(MPU6050_ADDR, 14, true);
  
  if (Wire.available() >= 14) {
    // Read gyroscope data (comes first)
    int16_t gxRaw = Wire.read() << 8 | Wire.read();
    int16_t gyRaw = Wire.read() << 8 | Wire.read();
    int16_t gzRaw = Wire.read() << 8 | Wire.read();
    
    // Skip temperature
    Wire.read();
    Wire.read();
    
    // Read accelerometer data (comes second)
    int16_t axRaw = Wire.read() << 8 | Wire.read();
    int16_t ayRaw = Wire.read() << 8 | Wire.read();
    int16_t azRaw = Wire.read() << 8 | Wire.read();
    
    // Convert to floating point (optional scaling)
    gx = gxRaw / 131.0; // Convert to deg/s
    gy = gyRaw / 131.0;
    gz = gzRaw / 131.0;
    
    ax = axRaw / 16384.0; // Convert to g
    ay = ayRaw / 16384.0;
    az = azRaw / 16384.0;
  }
}

// Print MPU6050 data
void printMPU6050Data(float ax, float ay, float az, float gx, float gy, float gz) {
  Serial.println("MPU6050 Data:");
  Serial.print("  Accel (g): X=");
  Serial.print(ax, 2);
  Serial.print(" Y=");
  Serial.print(ay, 2);
  Serial.print(" Z=");
  Serial.println(az, 2);
  
  Serial.print("  Gyro (deg/s): X=");
  Serial.print(gx, 2);
  Serial.print(" Y=");
  Serial.print(gy, 2);
  Serial.print(" Z=");
  Serial.println(gz, 2);
} 