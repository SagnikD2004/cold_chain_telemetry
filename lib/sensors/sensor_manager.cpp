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
        lis.setDataRate(LIS3DH_DATARATE_400_HZ);
    }
    
    return success;
}

void SensorManager::configureShockInterrupt(float threshold_g) {
    writeReg8(0x23, 0x38);

    writeReg8(0x21, 0x01);

    readReg8(0x26);

    uint8_t threshold_reg = (uint8_t)((threshold_g * 1000.0f) / 186.0f);
    writeReg8(LIS3DH_REG_INT1THS, threshold_reg & 0x7F);

    writeReg8(LIS3DH_REG_INT1DUR, 0x00);

    writeReg8(0x24, 0x08);

    writeReg8(LIS3DH_REG_CTRL3, 0x40);

    writeReg8(LIS3DH_REG_INT1CFG, 0x2A);

    clearInterrupt();
}

void SensorManager::clearInterrupt() {
    readReg8(LIS3DH_REG_INT1SRC);
}

float SensorManager::readVibrationBurstRMS(int num_samples) {
    float sum_sq = 0.0f;

    for (int i = 0; i < num_samples; i++) {
        sensors_event_t event;
        lis.getEvent(&event);
        
        float ax = event.acceleration.x / 9.80665f;
        float ay = event.acceleration.y / 9.80665f;
        float az = event.acceleration.z / 9.80665f;
        
        float total_mag = sqrt((ax * ax) + (ay * ay) + (az * az));
        float dyn_vib = fabs(total_mag - 1.0f);
        
        sum_sq += (dyn_vib * dyn_vib);
        
        delay(2); 
    }

    return sqrt(sum_sq / (float)num_samples);
}

bool SensorManager::readClimate(float &temp, float &hum) {
    sensors_event_t sht_hum, sht_temp;
    if (!sht4.getEvent(&sht_hum, &sht_temp)) return false;
    temp = sht_temp.temperature;
    hum = sht_hum.relative_humidity;
    return true;
}