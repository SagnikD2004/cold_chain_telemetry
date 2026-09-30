#include <Arduino.h>
#include "types.h"
#include "config.h"
#include "wifi_client.h"
#include "storage_manager.h"
#include "ble_beacon.h"

extern QueueHandle_t telemetryQueue;
extern WiFiUplink wifiClient;
extern StorageManager storageManager;
extern BLEBeacon bleBeacon;

void core0_comms_task(void *pvParameters) {
    uint8_t seq_counter = 0;
    uint32_t last_sync_time = millis();
    
    while (true) {
        TelemetryPacket packet;
        bool packet_received = false;
        
        if (xQueueReceive(telemetryQueue, &packet, pdMS_TO_TICKS(1000)) == pdTRUE) {
            packet_received = true;
            
            BLEPayload ble_frame;
            ble_frame.company_id = BLE_COMPANY_ID;
            ble_frame.temp_x100 = (int16_t)(packet.metrics.temperature * 100);
            ble_frame.hum_x100 = (uint16_t)(packet.metrics.humidity * 100);
            ble_frame.mkt_x100 = (int16_t)(packet.metrics.mkt * 100);
            ble_frame.vibe_rms_x1000 = (uint16_t)(packet.metrics.vibration_rms * 1000);
            ble_frame.battery_pct = packet.diagnostics.battery_pct;
            
            uint8_t alert_mask = 0;
            if (packet.alerts.temp_breach) alert_mask |= (1 << 0);
            if (packet.alerts.shock_detected) alert_mask |= (1 << 1);
            if (packet.ml_inference.anomaly_flag) alert_mask |= (1 << 2);
            ble_frame.alert_flags = alert_mask;
            ble_frame.seq_counter = seq_counter++;

            bleBeacon.updateAdvertisement(ble_frame);

            if (alert_mask > 0) {
                bleBeacon.start();
            } else {
                bleBeacon.stop();
            }
            
            CompactLogRecord compact;
            compact.timestamp = packet.timestamp;
            compact.temperature = packet.metrics.temperature;
            compact.humidity = packet.metrics.humidity;
            compact.vibration_rms = packet.metrics.vibration_rms;
            compact.mkt = packet.metrics.mkt;
            compact.alerts = alert_mask;
            
            storageManager.saveRecord(compact);
        }
        
        bool is_time_for_burst = (millis() - last_sync_time) >= INTERVAL_BURST_SYNC_MS;
        bool is_urgent_override = packet_received && packet.alerts.is_urgent;

        if (is_time_for_burst || is_urgent_override) {
            if (wifiClient.isConnected()) {
                CompactLogRecord batchBuffer[BATCH_MAX_UPLOAD_RECORDS];
                size_t retrieved = 0;
                
                if (storageManager.readBatch(batchBuffer, BATCH_MAX_UPLOAD_RECORDS, retrieved) && retrieved > 0) {
                    const char* n_id = packet_received ? packet.node_id : DEVICE_NODE_ID;
                    const char* t_id = packet_received ? packet.trip_id : DEFAULT_TRIP_ID;

                    if (wifiClient.postBatchSync(batchBuffer, retrieved, n_id, t_id)) {
                        storageManager.commitSync();
                    }
                }
            }
            last_sync_time = millis();
        }
    }
}