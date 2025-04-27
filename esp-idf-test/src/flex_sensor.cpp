#include <Arduino.h>

// Define the pins connected to the flex sensors
const int flexPin1 = 34; // Use an ADC capable pin (like GPIO 34)
const int flexPin2 = 35; // Use an ADC capable pin (like GPIO 35)

// Define the threshold for binary conversion
const int BINARY_THRESHOLD = 2048;

// Function to initialize flex sensors
void setupFlexSensors() {
  // No special setup needed for analog pins
  Serial.println("Flex Sensors initialized");
}

// Function to read flex sensor data
void readFlexSensors(int &flexValue1, int &flexValue2) {
  // Read analog values
  int rawValue1 = analogRead(flexPin1);
  int rawValue2 = analogRead(flexPin2);
  
  // Convert to binary (0 or 1) based on threshold
  flexValue1 = (rawValue1 > BINARY_THRESHOLD) ? 1 : 0;
  flexValue2 = (rawValue2 > BINARY_THRESHOLD) ? 1 : 0;
}

// Function to print flex sensor data
void printFlexSensorData(int flexValue1, int flexValue2) {
  Serial.print("Flex Sensor 1: ");
  Serial.print(flexValue1);
  Serial.print(" (Binary) | Flex Sensor 2: ");
  Serial.println(flexValue2);
} 