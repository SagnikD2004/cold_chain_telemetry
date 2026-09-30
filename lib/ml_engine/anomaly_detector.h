#pragma once
#include "types.h"

class AnomalyDetector {
public:
    bool begin();

    bool evaluateConditions(
        float temp_c, 
        float temp_gradient, 
        float humidity, 
        float vib_rms, 
        float dew_point, 
        MLInference &out_result
    );
};