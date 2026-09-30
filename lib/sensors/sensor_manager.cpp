#include "sensor_manager.h"
#include "config.h"

static void writeReg8(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(LIS3DH_DEFAULT_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static uint8_t readReg8(uint8_t reg) {
    Wire.beginTransmission(LIS3DH_DEFAULT_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)LIS3DH_DEFAULT_ADDRESS, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

bool SensorManager::begin(uint8_t int_pin) {
    lis_int_pin = int_pin;
    bool success = true;
    
    if (!sht4.begin()) {
        success = false;
    } else {
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
    }
    
    if (!lis.begin(LIS3DH_DEFAULT_ADDRESS)) { 
        success = false;
    } else {
        lis.setRange(LIS3DH_RANGE_16_G);        
        lis.setDataRate(LIS3DH_DATARATE_50_HZ); 
    }
    
    return success;
}

void SensorManager::configureShockInterrupt(float threshold_g) {

    uint8_t threshold_reg = (uint8_t)((threshold_g * 1000.0f) / 186.0f);
    
    writeReg8(LIS3DH_REG_INT1CFG, 0x3F); 
    writeReg8(LIS3DH_REG_INT1THS, threshold_reg & 0x7F); 
    writeReg8(LIS3DH_REG_INT1DUR, 0x02); 
    writeReg8(LIS3DH_REG_CTRL3, 0x40); 
}

void SensorManager::clearInterrupt() {
    readReg8(LIS3DH_REG_INT1SRC);
}

bool SensorManager::readSensors(float &temp, float &hum, float &accel_x, float &accel_y, float &accel_z) {
    sensors_event_t sht_hum, sht_temp, lis_event;
    
    if (!sht4.getEvent(&sht_hum, &sht_temp)) return false;
    lis.getEvent(&lis_event);
    
    temp = sht_temp.temperature;
    hum = sht_hum.relative_humidity;
    accel_x = lis_event.acceleration.x / 9.80665f;
    accel_y = lis_event.acceleration.y / 9.80665f;
    accel_z = lis_event.acceleration.z / 9.80665f;
    
    return true;
}