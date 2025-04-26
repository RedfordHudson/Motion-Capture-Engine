#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Create MPU6050 object
Adafruit_MPU6050 mpu;

// Function to initialize MPU6050
bool setupMPU6050() {
  // Try to initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    return false;
  }
  Serial.println("MPU6050 Found!");

  // Set accelerometer range to ±8G
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // Set gyro range to ±500 deg/s
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  // Set filter bandwidth to 21 Hz
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 initialized");
  return true;
}

// Function to read MPU6050 data
void readMPU6050(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  // Get new sensor events with the readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Assign values to the passed variables
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  gx = g.gyro.x;
  gy = g.gyro.y;
  gz = g.gyro.z;
}

// Function to print MPU6050 data
void printMPU6050Data(float ax, float ay, float az, float gx, float gy, float gz) {
  Serial.print("Acceleration X: ");
  Serial.print(ax);
  Serial.print(", Y: ");
  Serial.print(ay);
  Serial.print(", Z: ");
  Serial.print(az);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(gx);
  Serial.print(", Y: ");
  Serial.print(gy);
  Serial.print(", Z: ");
  Serial.print(gz);
  Serial.println(" rad/s");
} 