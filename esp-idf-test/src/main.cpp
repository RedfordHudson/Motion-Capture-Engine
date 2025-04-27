#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"
#include "audio_player.h"

// Variables to store sensor data
int flexValue1 = 0, flexValue2 = 0; // Binary values (0 or 1)
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;

// Create audio player instance
AudioPlayer audioPlayer;

// Variables to track previous sensor states
int prevFlex1 = -1;
int prevFlex2 = -1;

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  // Wait for Serial Monitor to open
  delay(1000);
  Serial.println("Multi-Sensor Reading Started");
  Serial.println("Flex sensors will output binary values (0 or 1) based on threshold of 2048");
  
  // Initialize I2C for MPU6050
  Wire.begin();
  
  // Initialize sensors
  setupFlexSensors();
  bool mpuSuccess = setupMPU6050();
  
  if (!mpuSuccess) {
    Serial.println("Warning: MPU6050 initialization failed. Continuing with flex sensors only.");
  }
  
  // Initialize audio player
  audioPlayer.setup();
}

void loop() {
  // Read data from all sensors
  readFlexSensors(flexValue1, flexValue2);
  
  // Try to read MPU6050 data (will only work if initialization was successful)
  readMPU6050(ax, ay, az, gx, gy, gz);
  
  // Check for sensor state changes and play appropriate audio
  if (flexValue1 == 1 && flexValue2 == 1) {
    // Both sensors are active (1,1)
    if (prevFlex1 != 1 || prevFlex2 != 1) {
      audioPlayer.playBothActive();
    }
  } else if (flexValue1 == 0 && flexValue2 == 0) {
    // Both sensors are inactive (0,0)
    if (prevFlex1 != 0 || prevFlex2 != 0) {
      audioPlayer.playBothInactive();
    }
  }
  
  // Update previous values
  prevFlex1 = flexValue1;
  prevFlex2 = flexValue2;
  
  // Update audio player
  audioPlayer.isPlaying();
  
  // Print all sensor data to serial monitor
  Serial.println("-----------------------------------");
  printFlexSensorData(flexValue1, flexValue2);
  printMPU6050Data(ax, ay, az, gx, gy, gz);
  Serial.println("-----------------------------------");
  
  // Small delay
  delay(200);
}
