#include <Arduino.h>
#include <Wire.h>
#include "sensors.h"

// Scan the I2C bus for devices
void scanI2C() {
  byte error, address;
  int deviceCount = 0;
  
  Serial.println("Scanning I2C bus...");
  
  for (address = 1; address < 128; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      
      // Check if it's an MPU6050 (address 0x68)
      if (address == 0x68) {
        Serial.print(" (Likely MPU6050)");
      }
      Serial.println();
      
      deviceCount++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.print("Scan complete. Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s)");
  }
  
  delay(5000); // Wait 5 seconds before scanning again
} 