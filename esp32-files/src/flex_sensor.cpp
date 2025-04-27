#include <Arduino.h>
#include "sensors.h"

// Flex sensor pins
#define FLEX_PIN_1 35  // GPIO35 (ADC1_CH7)
#define FLEX_PIN_2 34  // GPIO34 (ADC1_CH6)
#define FLEX_THRESHOLD 2048  // Threshold for binary detection

// Initialize flex sensors
void setupFlexSensor() {
  pinMode(FLEX_PIN_1, INPUT);
  pinMode(FLEX_PIN_2, INPUT);
  Serial.println("Flex sensors initialized on pins GPIO35 and GPIO34");
}

// Read value from flex sensor 1
int readFlexSensor1() {
  return analogRead(FLEX_PIN_1);
}

// Read value from flex sensor 2
int readFlexSensor2() {
  return analogRead(FLEX_PIN_2);
}

// Print flex sensor data
void printFlexSensorData(int flexValue1, int flexValue2) {
  Serial.print("Flex 1: Raw=");
  Serial.print(flexValue1);
  Serial.print(" (");
  Serial.print(flexValue1 > FLEX_THRESHOLD ? "BENT" : "STRAIGHT");
  Serial.print("), Flex 2: Raw=");
  Serial.print(flexValue2);
  Serial.print(" (");
  Serial.print(flexValue2 > FLEX_THRESHOLD ? "BENT" : "STRAIGHT");
  Serial.println(")");
} 