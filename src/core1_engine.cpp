#include <Arduino.h>
#include "types.h"
#include "config.h"
#include "sensor_manager.h"
#include "math_engine.h"
#include "anomaly_detector.h"

extern SensorManager sensorManager;
extern MathEngine mathEngine;
extern AnomalyDetector mlEngine;
extern QueueHandle_t telemetryQueue;
extern SemaphoreHandle_t wakeSemaphore;

void core1_sensing_task(void *pvParameters) {
    TickType_t xFrequency = pdMS_TO_TICKS(INTERVAL_NOMINAL_MS); 

    while (true) {
        bool shock_triggered = false;
        
        if (xSemaphoreTake(wakeSemaphore, xFrequency) == pdTRUE) {
            shock_triggered = true;
            sensorManager.clearInterrupt();
        }

        float temp, hum, ax, ay, az;
        if (!sensorManager.readSensors(temp, hum, ax, ay, az)) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; 
        }

        float rms = mathEngine.calculateVibrationRMS(ax, ay, az);
        float dew = mathEngine.calculateDewPoint(temp, hum);
        float grad = mathEngine.updateGradient(temp, millis());
        float mkt = mathEngine.updateMKT(temp);

        TelemetryPacket packet;
        strncpy(packet.node_id, DEVICE_NODE_ID, sizeof(packet.node_id));
        strncpy(packet.trip_id, DEFAULT_TRIP_ID, sizeof(packet.trip_id));
        packet.timestamp = millis() / 1000;

        packet.metrics = {temp, hum, dew, rms, grad, mkt};
        packet.diagnostics = {
            100, 
            4.2f, 
            SystemOperatingState::STATE_NORMAL_TRANSIT
        };

        mlEngine.evaluateConditions(temp, grad, hum, rms, dew, packet.ml_inference);

        packet.alerts.temp_breach = (temp < THRESHOLD_TEMP_MIN_C || temp > THRESHOLD_TEMP_MAX_C);
        packet.alerts.shock_detected = shock_triggered || (rms > THRESHOLD_SHOCK_G);
        
        packet.alerts.is_urgent = (packet.ml_inference.reconstruction_mse > THRESHOLD_ML_MSE) || 
                                  packet.alerts.temp_breach || 
                                  packet.alerts.shock_detected;

        if (packet.alerts.is_urgent) {
            xFrequency = pdMS_TO_TICKS(INTERVAL_EXCURSION_MS);
            packet.diagnostics.state = SystemOperatingState::STATE_ACTIVE_EXCURSION;
        } else {
            xFrequency = pdMS_TO_TICKS(INTERVAL_NOMINAL_MS);
            packet.diagnostics.state = SystemOperatingState::STATE_NORMAL_TRANSIT;
        }

        xQueueSend(telemetryQueue, &packet, 0);
    }
}