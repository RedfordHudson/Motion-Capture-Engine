#ifndef SENSORS_H
#define SENSORS_H

// Flex sensor functions
void setupFlexSensors();
void readFlexSensors(int &flexValue1, int &flexValue2);
void printFlexSensorData(int flexValue1, int flexValue2);

// MPU6050 functions
bool setupMPU6050();
void readMPU6050(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);
void printMPU6050Data(float ax, float ay, float az, float gx, float gy, float gz);

#endif 