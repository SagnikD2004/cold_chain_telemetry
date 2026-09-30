#include "storage_manager.h"
#include "config.h"
#include <LittleFS.h>

bool StorageManager::begin() {
    return LittleFS.begin(true);
}

bool StorageManager::saveRecord(const CompactLogRecord& record) {
    File file = LittleFS.open(STORAGE_BUFFER_FILE, FILE_APPEND);
    if (!file) return false;
    
    size_t bytes_written = file.write((const uint8_t*)&record, sizeof(CompactLogRecord));
    file.close();
    
    return bytes_written == sizeof(CompactLogRecord);
}

bool StorageManager::readBatch(CompactLogRecord* buffer, size_t max_records, size_t& out_retrieved_count) {
    out_retrieved_count = 0;
    
    if (!LittleFS.exists(STORAGE_BUFFER_FILE)) return false;
    
    File file = LittleFS.open(STORAGE_BUFFER_FILE, FILE_READ);
    if (!file) return false;
    
    while (file.available() && out_retrieved_count < max_records) {
        file.read((uint8_t*)&buffer[out_retrieved_count], sizeof(CompactLogRecord));
        out_retrieved_count++;
    }
    
    file.close();
    return out_retrieved_count > 0;
}

void StorageManager::commitSync() {
    if (LittleFS.exists(STORAGE_BUFFER_FILE)) {
        LittleFS.remove(STORAGE_BUFFER_FILE);
    }
}