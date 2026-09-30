#include <Arduino.h>
#include <Wire.h>
#include "esp_pm.h"
#include "esp_sleep.h"
#include "config.h"
#include "secrets.h"
#include "types.h"
#include "sensor_manager.h"
#include "math_engine.h"
#include "anomaly_detector.h"
#include "storage_manager.h"
#include "wifi_client.h"
#include "ble_beacon.h"

void core1_sensing_task(void *pvParameters);
void core0_comms_task(void *pvParameters);

SensorManager sensorManager;
MathEngine mathEngine;
AnomalyDetector mlEngine;
StorageManager storageManager;
WiFiUplink wifiClient;
BLEBeacon bleBeacon;

QueueHandle_t telemetryQueue;
SemaphoreHandle_t wakeSemaphore;

void IRAM_ATTR lis3dh_isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(wakeSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void setup() {
    Serial.begin(115200);
    
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    #if CONFIG_PM_ENABLE
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pm_config);
    #endif

    telemetryQueue = xQueueCreate(10, sizeof(TelemetryPacket));
    wakeSemaphore = xSemaphoreCreateBinary();
    
    sensorManager.begin(PIN_LIS3DH_INT);
    storageManager.begin();
    bleBeacon.begin();
    mlEngine.begin();
    
    sensorManager.configureShockInterrupt(THRESHOLD_SHOCK_G); 
    
    pinMode(PIN_LIS3DH_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_LIS3DH_INT), lis3dh_isr, RISING);
    
    gpio_wakeup_enable((gpio_num_t)PIN_LIS3DH_INT, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    wifiClient.begin(WIFI_SSID, WIFI_PASSWORD);

    xTaskCreatePinnedToCore(core1_sensing_task, "Sensing_Core1", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(core0_comms_task, "Comms_Core0", 16384, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelete(NULL);
}