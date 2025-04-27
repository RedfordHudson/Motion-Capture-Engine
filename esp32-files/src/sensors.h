#ifndef SENSORS_H
#define SENSORS_H

// I2C scanner functions
void scanI2C();

// Flex sensor functions
void setupFlexSensor();
int readFlexSensor1();
int readFlexSensor2();
void printFlexSensorData(int flexValue1, int flexValue2);

// MPU6050 functions
bool setupMPU6050();
void readMPU6050(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);
void printMPU6050Data(float ax, float ay, float az, float gx, float gy, float gz);

#endif 