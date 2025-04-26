#include <Arduino.h>

// Define the pins connected to the flex sensors
const int flexPin1 = 34; // Use an ADC capable pin (like GPIO 34)
const int flexPin2 = 35; // Use an ADC capable pin (like GPIO 35)

// Function to initialize flex sensors
void setupFlexSensors() {
  // No special setup needed for analog pins
  Serial.println("Flex Sensors initialized");
}

// Function to read flex sensor data
void readFlexSensors(int &flexValue1, int &flexValue2) {
  // Read analog values
  flexValue1 = analogRead(flexPin1);
  flexValue2 = analogRead(flexPin2);
}

// Function to print flex sensor data
void printFlexSensorData(int flexValue1, int flexValue2) {
  Serial.print("Flex Sensor 1: ");
  Serial.print(flexValue1);
  Serial.print(" | Flex Sensor 2: ");
  Serial.println(flexValue2);
} 