#pragma once
#include <stdint.h>

// --- Device & Trip Identifiers ---
#define DEVICE_NODE_ID            "NODE_001"
#define DEFAULT_TRIP_ID           "TRIP_ABC123"

// --- Hardware Pin Assignments ---
#define PIN_I2C_SDA               21
#define PIN_I2C_SCL               22
#define PIN_LIS3DH_INT            18

// --- FreeRTOS & System Timing (in Milliseconds) ---
#define INTERVAL_NOMINAL_MS       60000UL   // 60-second nominal sampling[cite: 2]
#define INTERVAL_EXCURSION_MS     5000UL    // 5-second accelerated sampling on alert[cite: 2]
#define INTERVAL_BURST_SYNC_MS    300000UL  // 5-minute Wi-Fi burst upload window

// --- Physical Alert & Quality Thresholds ---
#define THRESHOLD_TEMP_MIN_C      2.0f      // Lower critical bound[cite: 1]
#define THRESHOLD_TEMP_MAX_C      8.0f      // Upper critical bound[cite: 1]
#define THRESHOLD_SHOCK_G         2.5f      // ST LIS3DH drop detection trigger[cite: 1]
#define THRESHOLD_ML_MSE          0.045f    // TFLM Autoencoder reconstruction anomaly cutoff[cite: 1]

// --- LittleFS Ring Buffer Configuration ---
#define STORAGE_BUFFER_FILE       "/offline_sync.bin"
#define BATCH_MAX_UPLOAD_RECORDS  20        // Max 16-byte records processed per HTTP chunk[cite: 1]

// --- BLE Manufacturer Beacon Settings ---
#define BLE_BEACON_NAME           "COLD-CHAIN-NODE"
#define BLE_COMPANY_ID            0xFFFF    // Manufacturer Specific Data ID[cite: 1]
#define BLE_ADV_INTERVAL_TICKS    0x100     // 160 ms advertising interval