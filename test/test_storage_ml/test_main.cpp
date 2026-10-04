#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <unity.h>
#include <ArduinoJson.h>

#include "config.h"
#include "sensor_manager.h"
#include "math_engine.h"
#include "anomaly_detector.h"
#include "storage_manager.h"


static const int SAMPLE_COUNT = 20;
static const unsigned long SAMPLE_INTERVAL_MS = 1000;
static const int VIBRATION_SAMPLES = 50;

SensorManager sensorManager;
MathEngine mathEngine;
AnomalyDetector mlEngine;
StorageManager storageManager;

void setUp(void) {}
void tearDown(void) {}

void test_poll_infer_store_and_verify(void)
{
    // 1. Initialize Storage & Erase Old Data
    TEST_ASSERT_TRUE_MESSAGE(storageManager.begin(), "StorageManager initialization failed");
    storageManager.commitSync(); // Erases the existing STORAGE_BUFFER_FILE[cite: 7]

    // 2. Initialize Hardware & ML
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    TEST_ASSERT_TRUE_MESSAGE(sensorManager.begin(PIN_LIS3DH_INT), "Sensor initialization failed");
    TEST_ASSERT_TRUE_MESSAGE(mlEngine.begin(), "Anomaly detector initialization failed");

    // Initial reading to seed MathEngine dt calculations
    float temp_init,
        hum_init;
    if (sensorManager.readClimate(temp_init, hum_init))
    {
        mathEngine.updateGradient(temp_init, millis());
    }

    Serial.println("\n--- COLLECTING 20 READINGS ---");

    // 3. Polling Loop
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float temp, hum;
        TEST_ASSERT_TRUE_MESSAGE(sensorManager.readClimate(temp, hum), "SHT4x reading failed");

        uint32_t now = millis();
        float grad = mathEngine.updateGradient(temp, now);
        float vib_rms = sensorManager.readVibrationBurstRMS(VIBRATION_SAMPLES);
        float dew = mathEngine.calculateDewPoint(temp, hum);
        float current_mkt = mathEngine.updateMKT(temp, now);

        MLInference inf_result;
        bool ml_status = mlEngine.evaluateConditions(temp, grad, hum, vib_rms, dew, inf_result);
        TEST_ASSERT_TRUE_MESSAGE(ml_status, "ML evaluation failed");

        // Pack the bitmask
        uint8_t alert_bitmask = 0;
        if (inf_result.anomaly_flag)
        {
            alert_bitmask |= (1 << 0); // Bit 0: ML Anomaly detected
        }
        if (temp > THRESHOLD_TEMP_MAX_C || temp < THRESHOLD_TEMP_MIN_C)
        {
            alert_bitmask |= (1 << 1);
        }

        // Populate struct matching the schema
        CompactLogRecord record;
        memset(&record, 0, sizeof(CompactLogRecord));
        record.timestamp = now;
        record.temperature = temp;
        record.humidity = hum;
        record.vibration_rms = vib_rms;
        record.mkt = current_mkt;
        record.alerts = alert_bitmask;

        // Save to LittleFS
        TEST_ASSERT_TRUE_MESSAGE(storageManager.saveRecord(*(const struct CompactLogRecord *)&record), "Failed to save record");
        Serial.printf("Sample %d/20 stored. (Temp: %.2fC, Alerts: 0x%02X)\n", i + 1, temp, alert_bitmask);

        if (i < SAMPLE_COUNT - 1)
        {
            delay(SAMPLE_INTERVAL_MS);
        }
    }

    Serial.println("\n--- VERIFYING STORED DATA FROM LITTLEFS ---");

    CompactLogRecord read_buffer[SAMPLE_COUNT];
    size_t retrieved_count = 0;

    TEST_ASSERT_TRUE_MESSAGE(storageManager.readBatch((struct CompactLogRecord *)read_buffer, SAMPLE_COUNT, retrieved_count), "Read batch failed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(SAMPLE_COUNT, retrieved_count, "Did not retrieve exactly 20 records");

    // 5. Initialize JSON Document for Cloud Schema
    JsonDocument doc;
    doc["node_id"] = "ESP32_NODE_001";
    doc["trip_id"] = "TRIP_4920A";
    doc["sync_timestamp"] = millis();
    doc["record_count"] = retrieved_count;
    JsonArray recordsArray = doc["records"].to<JsonArray>();

    for (size_t i = 0; i < retrieved_count; i++)
    {
        Serial.printf("\n[Record %d]\n", i + 1);
        Serial.printf("  Timestamp: %u\n", read_buffer[i].timestamp);
        Serial.printf("  Temp     : %.4f\n", read_buffer[i].temperature);
        Serial.printf("  Humid    : %.4f\n", read_buffer[i].humidity);
        Serial.printf("  Vib RMS  : %.4f\n", read_buffer[i].vibration_rms);
        Serial.printf("  MKT      : %.4f\n", read_buffer[i].mkt);
        Serial.printf("  Alerts   : 0x%02X\n", read_buffer[i].alerts);

        // Print literal byte layout
        uint8_t *raw_bytes = (uint8_t *)&read_buffer[i];
        Serial.print("  Bytes    : ");
        for (size_t j = 0; j < sizeof(CompactLogRecord); j++)
        {
            Serial.printf("%02X ", raw_bytes[j]);
        }
        Serial.println();

        JsonObject recordObj = recordsArray.add<JsonObject>();
        recordObj["timestamp"] = read_buffer[i].timestamp;
        recordObj["temperature"] = serialized(String(read_buffer[i].temperature, 2));
        recordObj["humidity"] = serialized(String(read_buffer[i].humidity, 2));
        recordObj["vibration_rms"] = serialized(String(read_buffer[i].vibration_rms, 4));
        recordObj["mkt"] = serialized(String(read_buffer[i].mkt, 2));
        recordObj["alerts"] = read_buffer[i].alerts;
    }

    Serial.println("\n--- FINAL CLOUD JSON PAYLOAD ---");
    String outputJson;
    serializeJson(doc, outputJson);
    Serial.println(outputJson);
}

void setup()
{
    Serial.begin(115200);
    delay(2000); // Give the serial monitor time to attach
    
    Serial.println("\n--- STARTING TEST ---");
    Serial.println("If it hangs after this, LittleFS is formatting or I2C is stuck...");
    
    UNITY_BEGIN();
    RUN_TEST(test_poll_infer_store_and_verify);
    UNITY_END();
}

void loop() {}