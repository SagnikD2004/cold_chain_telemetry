#pragma once
#include <Adafruit_SHT4x.h>
#include <Adafruit_LIS3DH.h>
#include <Wire.h>

class SensorManager {
public:
    bool begin(uint8_t int_pin);
    
    void configureShockInterrupt(float threshold_g);
    
    void clearInterrupt();
    
    bool readSensors(float &temp, float &hum, float &accel_x, float &accel_y, float &accel_z);

private:
    Adafruit_SHT4x sht4;
    Adafruit_LIS3DH lis;
    uint8_t lis_int_pin;
};