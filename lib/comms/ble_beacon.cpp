#include "ble_beacon.h"
#include <Arduino.h>

bool BLEBeacon::begin() {
    Serial.println("[STUB-BLE] Mock BLE Beacon initialized (Radio advertising disabled)");
    return true;
}

void BLEBeacon::start() {
    Serial.println("[STUB-BLE] Advertising started (Mock)");
}

void BLEBeacon::stop() {
    Serial.println("[STUB-BLE] Advertising stopped (Mock)");
}

void BLEBeacon::updateAdvertisement(const BLEPayload& payload) {
    
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&payload);
    
    Serial.print("[STUB-BLE] 13-Byte Frame Broadcast: ");
    for (size_t i = 0; i < sizeof(BLEPayload); i++) {
        if (raw[i] < 0x10) Serial.print("0");
        Serial.print(raw[i], HEX);
        Serial.print(" ");
    }
    Serial.printf("| Temp*100:%d AlertMask:0x%02X Seq:%u\n", 
                  payload.temp_x100, payload.alert_flags, payload.seq_counter); //[cite: 1]
}