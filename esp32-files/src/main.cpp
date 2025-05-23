#include <Arduino.h>
#include <Wire.h>
#include "esp_log.h"

// Mode selection (true = scan for I2C devices, false = read MPU6050 data)
bool scanMode = false;

static const char *TAG = "MPU6050";

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// MPU6050 register addresses
#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_GYRO_START   0x3B  // Gyroscope data comes first
#define MPU6050_ACCEL_START  0x43  // Accelerometer data comes second

// Function to write a byte to a register
void writeRegister(uint8_t reg_addr, uint8_t data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg_addr);
  Wire.write(data);
  Wire.endTransmission();
}

// Function to read multiple bytes from registers
void readRegisters(uint8_t start_reg, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(start_reg);
  Wire.endTransmission(false); // false = don't send stop condition
  
  // Request length bytes from the MPU6050
  Wire.requestFrom(MPU6050_ADDR, length);
  
  // Read the data into the buffer
  size_t i = 0;
  while (Wire.available() && i < length) {
    buffer[i++] = Wire.read();
  }
}

// Function to combine two bytes into a 16-bit value
int16_t combineBytes(uint8_t high, uint8_t low) {
  return (int16_t)((high << 8) | low);
}

// Function to scan I2C bus
void scanI2C() {
  byte error, address;
  int deviceCount = 0;
  
  ESP_LOGI(TAG, "Scanning I2C bus...");
  
  for (address = 1; address < 128; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      ESP_LOGI(TAG, "I2C device found at address 0x%02X", address);
      deviceCount++;
      
      // Check if it's an MPU6050 (address 0x68)
      if (address == 0x68) {
        ESP_LOGI(TAG, "This could be an MPU6050!");
      }
    } else if (error == 4) {
      ESP_LOGE(TAG, "Unknown error at address 0x%02X", address);
    }
  }
  
  if (deviceCount == 0) {
    ESP_LOGE(TAG, "No I2C devices found");
  } else {
    ESP_LOGI(TAG, "Scan complete. Found %d device(s)", deviceCount);
  }
  
  delay(5000); // Wait 5 seconds before scanning again
}

// Function to read and output MPU6050 sensor data
void readMPU6050() {
  uint8_t data[14]; // 6 gyro + 2 temp + 6 accel
  
  // Read all sensor data at once (14 bytes)
  readRegisters(MPU6050_GYRO_START, data, 14);
  
  // Process gyroscope data (comes first)
  int16_t gx = combineBytes(data[0], data[1]);
  int16_t gy = combineBytes(data[2], data[3]);
  int16_t gz = combineBytes(data[4], data[5]);
  
  // Process accelerometer data (comes second)
  int16_t ax = combineBytes(data[8], data[9]);
  int16_t ay = combineBytes(data[10], data[11]);
  int16_t az = combineBytes(data[12], data[13]);
  
  // Print formatted serial packet (JSON format)
  Serial.printf("{\"ax\":%d,\"ay\":%d,\"az\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d}\n", 
               ax, ay, az, gx, gy, gz);
  
  delay(20); // 50Hz (20ms delay)
}

void setup() {
  Serial.begin(115200);
  ESP_LOGI(TAG, "Initializing I2C...");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!scanMode) {
    // Initialize MPU6050 if in read mode
    writeRegister(MPU6050_PWR_MGMT_1, 0x00);
    ESP_LOGI(TAG, "MPU6050 initialized and awake");
  } else {
    ESP_LOGI(TAG, "I2C Scanner ready");
    delay(1000); // Give devices time to power up
  }
}

void loop() {
  if (scanMode) {
    scanI2C();
  } else {
    readMPU6050();
  }
}
