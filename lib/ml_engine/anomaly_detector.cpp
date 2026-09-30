#include "anomaly_detector.h"
#include <Arduino.h>
#include <math.h>

bool AnomalyDetector::begin() {
    Serial.println("[STUB-ML] Mock AnomalyDetector initialized (Static 4KB arena bypassed)");
    return true;
}

bool AnomalyDetector::evaluateConditions(
    float temp_c, 
    float temp_gradient, 
    float humidity, 
    float vib_rms, 
    float dew_point, 
    MLInference &out_result
) {
    // Normal operation mock: Nominal baseline between 2°C and 8°C[cite: 1]
    // Simulates low MSE during nominal conditions, elevated MSE during spikes
    if (temp_c < 2.0f || temp_c > 8.0f || fabsf(temp_gradient) > 1.5f || vib_rms > 2.0f) {
        out_result.reconstruction_mse = 0.062f; // Exceeds 0.045 threshold[cite: 1]
        out_result.anomaly_flag = true;         //[cite: 1]
    } else {
        out_result.reconstruction_mse = 0.015f; // Nominal baseline[cite: 1]
        out_result.anomaly_flag = false;        //[cite: 1]
    }

    Serial.printf("[STUB-ML] Ingested T:%.2f Grad:%.2f RH:%.2f -> MSE:%.4f (Anomaly:%s)\n",
                  temp_c, temp_gradient, humidity, 
                  out_result.reconstruction_mse, 
                  out_result.anomaly_flag ? "TRUE" : "FALSE");

    return true;
}