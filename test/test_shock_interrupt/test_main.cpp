#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include "sensor_manager.h"

#define PIN_LED_BLUE   2
#define PIN_LIS_INT1   18   // LIS3DH I1 connected to GPIO18
#define SHOCK_THRESH_G 2.0f

SensorManager sensorManager;
SemaphoreHandle_t shockSemaphore;
volatile uint32_t isr_trigger_count = 0;

void IRAM_ATTR shock_isr() {
    isr_trigger_count++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(shockSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void setUp(void) {}
void tearDown(void) {}

void test_shock_live_monitor(void) {
    Wire.begin(21, 22);

    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, LOW);

    pinMode(PIN_LIS_INT1, INPUT_PULLDOWN);

    bool init_ok = sensorManager.begin(PIN_LIS_INT1);
    TEST_ASSERT_TRUE_MESSAGE(init_ok, "LIS3DH initialization failed!");

    // Configure interrupt & print registers
    sensorManager.configureShockInterrupt(SHOCK_THRESH_G);
    // Check baseline pin state while sitting still
    delay(200);
    int initial_pin_state = digitalRead(PIN_LIS_INT1);
    Serial.printf("Initial GPIO18 Level (Stationary): %s\n", 
                  initial_pin_state == HIGH ? "HIGH (ASSERTED)" : "LOW (IDLE)");

    shockSemaphore = xSemaphoreCreateBinary();
    attachInterrupt(digitalPinToInterrupt(PIN_LIS_INT1), shock_isr, RISING);

    Serial.println("Starting 15-second monitoring window...");

    uint32_t start_time = millis();
    while (millis() - start_time < 15000) {
        float burst_rms = sensorManager.readVibrationBurstRMS(20);

        if (xSemaphoreTake(shockSemaphore, 0) == pdTRUE) {
            // Flash LED
            digitalWrite(PIN_LED_BLUE, HIGH);
            delay(120);
            digitalWrite(PIN_LED_BLUE, LOW);

            // Read INT1_SRC to deassert hardware latch
            sensorManager.clearInterrupt();

            Serial.printf("\n>>> [SHOCK DETECTED] Count: %u | RMS: %.2f g <<<\n\n", 
                          isr_trigger_count, burst_rms);
        } else {
            Serial.printf("Force: %4.2f g | GPIO18: %d\n", burst_rms, digitalRead(PIN_LIS_INT1));
        }

        delay(100);
    }

    detachInterrupt(digitalPinToInterrupt(PIN_LIS_INT1));
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_shock_live_monitor);
    UNITY_END();
}

void loop() {}