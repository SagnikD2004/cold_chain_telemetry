#pragma once
#include <stdint.h>

// Firmware state machine status on Core 1[cite: 1]
enum class SystemOperatingState : uint8_t {
    STATE_STABLE_CRUISE = 0,
    STATE_NORMAL_TRANSIT,
    STATE_ACTIVE_EXCURSION,
    STATE_DOOR_BREACH
};

// Maps to telemetry_metrics schema[cite: 1]
struct TelemetryMetrics {
    float temperature;   // -40.0 to 85.0 °C[cite: 1]
    float humidity;      // 0.0 to 100.0 %[cite: 1]
    float dew_point;     // Magnus formula dew point[cite: 1]
    float vibration_rms; // 0.0 to 16.0 Gs[cite: 1]
    float temp_gradient; // dT/dt in °C/min[cite: 1]
    float mkt;           // Haynes Equation MKT[cite: 1]
};

// Maps to diagnostics schema[cite: 1]
struct Diagnostics {
    uint8_t battery_pct;        // 0 to 100%[cite: 1]
    float battery_voltage;      // 2.5V to 4.5V[cite: 1]
    SystemOperatingState state; 
};

// Maps to ml_inference schema[cite: 1]
struct MLInference {
    float reconstruction_mse; // TFLM Autoencoder MSE[cite: 1]
    bool anomaly_flag;        // True if MSE > 0.045[cite: 1]
};

// Maps to alerts schema[cite: 1]
struct Alerts {
    bool is_urgent;      // Immediate transmission signal[cite: 1]
    bool temp_breach;    // Outside 2-8°C or fast ramp[cite: 1]
    bool shock_detected; // >2.5G acceleration spike[cite: 1]
};

// FreeRTOS IPC Payload for Core 0 JSON serialization (POST /api/v1/telemetry)[cite: 1]
struct TelemetryPacket {
    char node_id[33]; 
    char trip_id[33];
    uint32_t timestamp;       // Epoch or uptime tick[cite: 1]
    TelemetryMetrics metrics;
    Diagnostics diagnostics;
    MLInference ml_inference;
    Alerts alerts;
};

#pragma pack(push, 1) // Force 1-byte alignment for flash/BLE serialization

// Maps to batch_record schema for LittleFS offline ring buffer[cite: 1]
struct CompactLogRecord {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float vibration_rms;
    float mkt;
    uint8_t alerts; // Packed bitmask from LittleFS[cite: 1]
};

// Maps to ble_binary_packet schema: Exactly 13 bytes[cite: 1]
struct BLEPayload {
    uint16_t company_id;     // 0xFFFF[cite: 1]
    int16_t temp_x100;       // Temp * 100[cite: 1]
    uint16_t hum_x100;       // RH * 100[cite: 1]
    int16_t mkt_x100;        // MKT * 100[cite: 1]
    uint16_t vibe_rms_x1000; // Gs * 1000[cite: 1]
    uint8_t battery_pct;     // 0 to 100[cite: 1]
    uint8_t alert_flags;     // Bit0=Temp, Bit1=Motion, Bit2=ML, Bit3=Battery[cite: 1]
    uint8_t seq_counter;     // 0..255 rollover[cite: 1]
};
#pragma pack(pop)