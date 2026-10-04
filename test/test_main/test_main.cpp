// #include <Arduino.h>
// #include <unity.h>
// #include <Wire.h>
// #include "sensor_manager.h"

// #define PIN_LED_BLUE   2
// #define PIN_LIS_INT1   18   // LIS3DH I1 connected to GPIO18
// #define SHOCK_THRESH_G 2.0f

// SensorManager sensorManager;
// SemaphoreHandle_t shockSemaphore;
// volatile uint32_t isr_trigger_count = 0;

// void IRAM_ATTR shock_isr() {
//     isr_trigger_count++;
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     xSemaphoreGiveFromISR(shockSemaphore, &xHigherPriorityTaskWoken);
//     if (xHigherPriorityTaskWoken == pdTRUE) {
//         portYIELD_FROM_ISR();
//     }
// }

// void setUp(void) {}
// void tearDown(void) {}

// void test_shock_live_monitor(void) {
//     Wire.begin(21, 22);

//     pinMode(PIN_LED_BLUE, OUTPUT);
//     digitalWrite(PIN_LED_BLUE, LOW);

//     pinMode(PIN_LIS_INT1, INPUT_PULLDOWN);

//     bool init_ok = sensorManager.begin(PIN_LIS_INT1);
//     TEST_ASSERT_TRUE_MESSAGE(init_ok, "LIS3DH initialization failed!");

//     // Configure interrupt & print registers
//     sensorManager.configureShockInterrupt(SHOCK_THRESH_G);
//     // Check baseline pin state while sitting still
//     delay(200);
//     int initial_pin_state = digitalRead(PIN_LIS_INT1);
//     Serial.printf("Initial GPIO18 Level (Stationary): %s\n", 
//                   initial_pin_state == HIGH ? "HIGH (ASSERTED)" : "LOW (IDLE)");

//     shockSemaphore = xSemaphoreCreateBinary();
//     attachInterrupt(digitalPinToInterrupt(PIN_LIS_INT1), shock_isr, RISING);

//     Serial.println("Starting 15-second monitoring window...");

//     uint32_t start_time = millis();
//     while (millis() - start_time < 15000) {
//         float burst_rms = sensorManager.readVibrationBurstRMS(20);

//         if (xSemaphoreTake(shockSemaphore, 0) == pdTRUE) {
//             // Flash LED
//             digitalWrite(PIN_LED_BLUE, HIGH);
//             delay(120);
//             digitalWrite(PIN_LED_BLUE, LOW);

//             // Read INT1_SRC to deassert hardware latch
//             sensorManager.clearInterrupt();

//             Serial.printf("\n>>> [SHOCK DETECTED] Count: %u | RMS: %.2f g <<<\n\n", 
//                           isr_trigger_count, burst_rms);
//         } else {
//             Serial.printf("Force: %4.2f g | GPIO18: %d\n", burst_rms, digitalRead(PIN_LIS_INT1));
//         }

//         delay(100);
//     }

//     detachInterrupt(digitalPinToInterrupt(PIN_LIS_INT1));
// }

// void setup() {
//     delay(2000);
//     Serial.begin(115200);

//     UNITY_BEGIN();
//     RUN_TEST(test_shock_live_monitor);
//     UNITY_END();
// }

// void loop() {}
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "sensor_manager.h"
#include "math_engine.h"
#include "anomaly_detector.h"


// ============================================================
// OBJECTS
// ============================================================

SensorManager sensorManager;
MathEngine mathEngine;
AnomalyDetector mlEngine;


// ============================================================
// CONFIGURATION
// ============================================================

const unsigned long SAMPLE_INTERVAL_MS = 10000;

const int VIBRATION_SAMPLES = 50;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(2000);

    Serial.println();
    Serial.println("==========================================");
    Serial.println(" ESP32 LIVE SENSOR + ML TEST");
    Serial.println("==========================================");


    // ========================================================
    // I2C
    // ========================================================

    Serial.println();
    Serial.println("Initializing I2C...");

    Wire.begin(
        PIN_I2C_SDA,
        PIN_I2C_SCL
    );

    Serial.println("I2C initialized.");


    // ========================================================
    // SENSOR INITIALIZATION
    // ========================================================

    Serial.println();
    Serial.println("Initializing sensors...");

    if (!sensorManager.begin(PIN_LIS3DH_INT))
    {
        Serial.println("ERROR: Sensor initialization failed!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("SHT4x initialized.");
    Serial.println("LIS3DH initialized.");


    // ========================================================
    // ML INITIALIZATION
    // ========================================================

    Serial.println();
    Serial.println("Initializing ML engine...");

    if (!mlEngine.begin())
    {
        Serial.println("ERROR: ML initialization failed!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("ML engine initialized.");


    // ========================================================
    // INITIAL TEMPERATURE READING
    //
    // Needed to calculate dT/dt.
    // This reading is NOT sent to ML.
    // ========================================================

    float temperature;
    float humidity;

    if (sensorManager.readClimate(
            temperature,
            humidity))
    {
        mathEngine.updateGradient(
            temperature,
            millis()
        );

        Serial.println();
        Serial.printf(
            "Initial temperature: %.4f C\n",
            temperature
        );
    }
    else
    {
        Serial.println(
            "WARNING: Initial climate read failed."
        );
    }


    Serial.println();
    Serial.println("==========================================");
    Serial.println(" LIVE ML TEST STARTED");
    Serial.println(" Sampling interval : 10 seconds");
    Serial.println(" Vibration samples : 50");
    Serial.println("==========================================");
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // ========================================================
    // 1. READ SHT4x
    // ========================================================

    float temperature;
    float humidity;

    if (!sensorManager.readClimate(
            temperature,
            humidity))
    {
        Serial.println(
            "ERROR: SHT4x read failed!"
        );

        delay(SAMPLE_INTERVAL_MS);

        return;
    }


    // ========================================================
    // 2. CURRENT TIME
    // ========================================================

    uint32_t current_time = millis();


    // ========================================================
    // 3. TEMPERATURE GRADIENT
    // ========================================================

    float temp_gradient =
        mathEngine.updateGradient(
            temperature,
            current_time
        );


    // ========================================================
    // 4. VIBRATION RMS
    // ========================================================

    Serial.println();
    Serial.println(
        "Collecting vibration data..."
    );

    float vibration_rms =
        sensorManager.readVibrationBurstRMS(
            VIBRATION_SAMPLES
        );


    // ========================================================
    // 5. DEW POINT
    // ========================================================

    float dew_point =
        mathEngine.calculateDewPoint(
            temperature,
            humidity
        );


    // ========================================================
    // 6. PRINT RAW FEATURE VECTOR
    // ========================================================

    Serial.println();
    Serial.println(
        "=========================================="
    );

    Serial.println(
        "RAW SENSOR FEATURE VECTOR"
    );

    Serial.println(
        "=========================================="
    );

    Serial.printf(
        "Temperature     : %.6f C\n",
        temperature
    );

    Serial.printf(
        "Temp Gradient   : %.8f C/s\n",
        temp_gradient
    );

    Serial.printf(
        "Humidity        : %.6f %%RH\n",
        humidity
    );

    Serial.printf(
        "Vibration RMS   : %.8f G\n",
        vibration_rms
    );

    Serial.printf(
        "Dew Point       : %.6f C\n",
        dew_point
    );


    // ========================================================
    // 7. SEND TO ACTUAL ML ENGINE
    // ========================================================

    MLInference result;

    bool success =
        mlEngine.evaluateConditions(
            temperature,
            temp_gradient,
            humidity,
            vibration_rms,
            dew_point,
            result
        );


    if (!success)
    {
        Serial.println(
            "ERROR: ML inference failed!"
        );

        delay(SAMPLE_INTERVAL_MS);

        return;
    }

// ========================================================
// 8. PRINT MSE
// ========================================================

Serial.println();
Serial.println(
    "=========================================="
);

Serial.println(
    "ML RESULT"
);

Serial.println(
    "=========================================="
);

Serial.printf(
    "MSE       : %.10f\n",
    result.reconstruction_mse
);

Serial.printf(
    "Threshold : %.10f\n",
    THRESHOLD_ML_MSE
);

if (result.anomaly_flag)
{
    Serial.println(
        "RESULT    : ANOMALY"
    );
}
else
{
    Serial.println(
        "RESULT    : NORMAL"
    );
}


    // ========================================================
    // 9. WAIT
    // ========================================================

    Serial.println();
    Serial.println(
        "Next ML sample in 10 seconds..."
    );

    delay(SAMPLE_INTERVAL_MS);
}