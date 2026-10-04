#include <Arduino.h>
#include <Wire.h>
#include <unity.h> // Required for PlatformIO testing

#include "config.h"
#include "sensor_manager.h"
#include "math_engine.h"
#include "anomaly_detector.h"

SensorManager sensorManager;
MathEngine mathEngine;
AnomalyDetector mlEngine;

const unsigned long SAMPLE_INTERVAL_MS = 10000;
const int VIBRATION_SAMPLES = 50;
const int MAX_READINGS = 10;

// ============================================================
// UNITY REQUIREMENTS
// ============================================================
void setUp(void) {
    // Run before each test (can be empty)
}

void tearDown(void) {
    // Run after each test (can be empty)
}

// ============================================================
// TEST CASE
// ============================================================
void test_live_ml_inference(void) {
    Serial.println("\nInitializing I2C...");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    Serial.println("Initializing sensors...");
    // TEST_ASSERT_TRUE will immediately fail the test if the condition is false
    TEST_ASSERT_TRUE_MESSAGE(sensorManager.begin(PIN_LIS3DH_INT), "Sensor initialization failed!");

    Serial.println("Initializing ML engine...");
    TEST_ASSERT_TRUE_MESSAGE(mlEngine.begin(), "ML initialization failed!");

    float temperature;
    float humidity;
    if (sensorManager.readClimate(temperature, humidity)) {
        mathEngine.updateGradient(temperature, millis());
        Serial.printf("\nInitial temperature: %.4f C\n", temperature);
    } else {
        TEST_FAIL_MESSAGE("Initial climate read failed.");
    }

    Serial.println("\n==========================================");
    Serial.println(" LIVE ML TEST STARTED");
    Serial.printf(" Target readings   : %d\n", MAX_READINGS);
    Serial.println(" Sampling interval : 10 seconds");
    Serial.println("==========================================");

    // Loop 10 times synchronously within the test
    for (int i = 1; i <= MAX_READINGS; i++) {
        Serial.printf("\n--- Reading %d of %d ---\n", i, MAX_READINGS);

        TEST_ASSERT_TRUE_MESSAGE(sensorManager.readClimate(temperature, humidity), "SHT4x read failed!");

        uint32_t current_time = millis();
        float temp_gradient = mathEngine.updateGradient(temperature, current_time);

        float vibration_rms = sensorManager.readVibrationBurstRMS(VIBRATION_SAMPLES);
        float dew_point = mathEngine.calculateDewPoint(temperature, humidity);

        Serial.printf("Temp: %.2fC | Grad: %.4fC/s | Hum: %.2f%% | Vib: %.4fG | Dew: %.2fC\n",
                      temperature, temp_gradient, humidity, vibration_rms, dew_point);

        MLInference result;
        bool success = mlEngine.evaluateConditions(
            temperature, temp_gradient, humidity, vibration_rms, dew_point, result
        );
        
        TEST_ASSERT_TRUE_MESSAGE(success, "ML inference failed!");

        Serial.printf("MSE: %.10f | Threshold: %.10f | RESULT: %s\n", 
                      result.reconstruction_mse, 
                      THRESHOLD_ML_MSE, 
                      result.anomaly_flag ? "ANOMALY" : "NORMAL");

        // Wait before next sample, unless it's the last one
        if (i < MAX_READINGS) {
            delay(SAMPLE_INTERVAL_MS);
        }
    }
}

// ============================================================
// MAIN
// ============================================================
void setup() {
    delay(2000); // Give the serial monitor time to connect

    UNITY_BEGIN();
    
    // Execute our test function
    RUN_TEST(test_live_ml_inference);
    
    UNITY_END(); // Prints final test summary
}

void loop() {
    // Empty: Unity framework tests run strictly inside setup()
}