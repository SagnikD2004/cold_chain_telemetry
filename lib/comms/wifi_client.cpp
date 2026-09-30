#include "wifi_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define API_REALTIME "http://YOUR_SERVER_IP/api/v1/telemetry"
#define API_BATCH    "http://YOUR_SERVER_IP/api/v1/telemetry/batch"

bool WiFiUplink::begin(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Wait for connection (implement timeout in production)
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    return true;
}

bool WiFiUplink::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiUplink::postRealtime(const TelemetryPacket& packet) {
    if (!isConnected()) return false;
    
    HTTPClient http;
    http.begin(API_REALTIME);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<512> doc;
    doc["node_id"] = packet.node_id;
    doc["trip_id"] = packet.trip_id;
    doc["timestamp"] = packet.timestamp;
    
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["temperature"] = packet.metrics.temperature;
    metrics["humidity"] = packet.metrics.humidity;
    metrics["dew_point"] = packet.metrics.dew_point;
    metrics["vibration_rms"] = packet.metrics.vibration_rms;
    metrics["temp_gradient"] = packet.metrics.temp_gradient;
    metrics["mkt"] = packet.metrics.mkt;
    
    JsonObject diag = doc.createNestedObject("diagnostics");
    diag["battery_pct"] = packet.diagnostics.battery_pct;
    diag["battery_voltage"] = packet.diagnostics.battery_voltage;
    diag["operating_state"] = "STATE_NORMAL_TRANSIT";
    
    JsonObject ml = doc.createNestedObject("ml_inference");
    ml["reconstruction_mse"] = packet.ml_inference.reconstruction_mse;
    ml["anomaly_flag"] = packet.ml_inference.anomaly_flag;
    
    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["is_urgent"] = packet.alerts.is_urgent;
    alerts["temp_breach"] = packet.alerts.temp_breach;
    alerts["shock_detected"] = packet.alerts.shock_detected;
    
    String payload;
    serializeJson(doc, payload);
    
    int httpResponseCode = http.POST(payload);
    http.end();
    
    return (httpResponseCode == 200 || httpResponseCode == 201);
}

bool WiFiUplink::postBatchSync(const CompactLogRecord* records, size_t count, const char* node_id, const char* trip_id) {
    if (!isConnected() || count == 0) return false;
    
    HTTPClient http;
    http.begin(API_BATCH);
    http.addHeader("Content-Type", "application/json");
    
    DynamicJsonDocument doc(1024 + (count * 128)); 
    doc["node_id"] = node_id;
    doc["trip_id"] = trip_id;
    doc["sync_timestamp"] = millis(); 
    doc["record_count"] = count;
    
    JsonArray json_records = doc.createNestedArray("records");
    for (size_t i = 0; i < count; i++) {
        JsonObject rec = json_records.createNestedObject();
        rec["timestamp"] = records[i].timestamp;
        rec["temperature"] = records[i].temperature;
        rec["humidity"] = records[i].humidity;
        rec["vibration_rms"] = records[i].vibration_rms;
        rec["mkt"] = records[i].mkt;
        rec["alerts"] = records[i].alerts;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    int httpResponseCode = http.POST(payload);
    http.end();
    
    return (httpResponseCode == 200 || httpResponseCode == 201);
}