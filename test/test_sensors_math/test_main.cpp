#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include "sensor_manager.h"
#include "math_engine.h"

SensorManager sensorManager;
MathEngine mathEngine;


#ifndef PIN_I2C_SDA
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#endif
#define LIS_INT_PIN 18

void setUp(void) {
    // Runs before each test
}

void tearDown(void) {
    // Runs after each test
}

void test_initialization(void) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    bool init_ok = sensorManager.begin(LIS_INT_PIN); 
    TEST_ASSERT_TRUE_MESSAGE(init_ok, "Sensor initialization failed! Check I2C wiring.");
}

void test_csv_pipeline(void) {
    Serial.println("timestamp_ms,temp_c,humidity_pct,vibration_rms_g,dew_point_c,temp_gradient_c_per_min,mkt_c");

    for (int i = 0; i < 1000; i++) {
        float temp = 0.0f, hum = 0.0f;
        float ax, ay, az; 
        
        // sensorManager.readSensors(temp, hum, ax, ay, az);
        
        float rms = sensorManager.readVibrationBurstRMS(50);
        
        uint32_t current_time = millis();
        
        float dp   = mathEngine.calculateDewPoint(temp, hum);
        float grad = mathEngine.updateGradient(temp, current_time);
        float mkt  = mathEngine.updateMKT(temp);

        Serial.printf("%lu,%.2f,%.2f,%.3f,%.2f,%.4f,%.2f\n",
                      current_time, temp, hum, rms, dp, grad, mkt);

        delay(100); 
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_csv_pipeline);
    UNITY_END();
}

void loop() {
    // Test completed, do nothing
}