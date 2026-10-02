#pragma once
#include <stdint.h>
#include <BLEDevice.h>
#include "types.h"

class BLEBeacon {
public:
    bool begin();
    void updateAdvertisement(const BLEPayload& payload);
    void start();
    void stop();
};