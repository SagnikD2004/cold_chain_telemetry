#pragma once
#include "types.h"
#include <stddef.h>

class WiFiUplink {
public:
    bool begin(const char* ssid, const char* password);
    bool isConnected();

    bool postRealtime(const TelemetryPacket& packet);
    
    bool postBatchSync(const CompactLogRecord* records, size_t count, const char* node_id, const char* trip_id);
};