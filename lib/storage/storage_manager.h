#pragma once
#include "types.h"
#include <stddef.h>

class StorageManager {
public:
    bool begin();
    
    bool saveRecord(const CompactLogRecord& record);
    
    bool readBatch(CompactLogRecord* buffer, size_t max_records, size_t& out_retrieved_count);
    
    void commitSync(); 
};