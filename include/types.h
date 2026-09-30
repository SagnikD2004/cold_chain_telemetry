#pragma once
#include <stdint.h>

enum class SystemOperatingState : uint8_t {
    STATE_STABLE_CRUISE = 0,
    STATE_NORMAL_TRANSIT,
    STATE_ACTIVE_EXCURSION,
    STATE_DOOR_BREACH
};

struct TelemetryMetrics {
    float temperature;   
    float humidity;      
    float dew_point;     
    float vibration_rms; 
    float temp_gradient; 
    float mkt;           
};

struct Diagnostics {
    uint8_t battery_pct;        
    float battery_voltage;      
    SystemOperatingState state; 
};

struct MLInference {
    float reconstruction_mse; 
    bool anomaly_flag;        
};

struct Alerts {
    bool is_urgent;      
    bool temp_breach;    
    bool shock_detected; 
};

struct TelemetryPacket {
    char node_id[33]; 
    char trip_id[33];
    uint32_t timestamp;       
    TelemetryMetrics metrics;
    Diagnostics diagnostics;
    MLInference ml_inference;
    Alerts alerts;
};

#pragma pack(push, 1) 
struct CompactLogRecord {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float vibration_rms;
    float mkt;
    uint8_t alerts; 
};

struct BLEPayload {
    uint16_t company_id;   
    int16_t temp_x100;       
    uint16_t hum_x100;      
    int16_t mkt_x100;        
    uint16_t vibe_rms_x1000; 
    uint8_t battery_pct;     
    uint8_t alert_flags;     
    uint8_t seq_counter;   
};
#pragma pack(pop)