#pragma once
#include <stdint.h>
#include <math.h>

class MathEngine {
public:
    MathEngine();
    
    float calculateDewPoint(float temp_c, float humidity_pct);
    float calculateVibrationRMS(float x_g, float y_g, float z_g);
    float updateGradient(float current_temp_c, uint32_t current_time_ms);
    float updateMKT(float current_temp_c);

private:
    // Cumulative MKT state variables
    double exp_sum;
    uint32_t sample_count;
    
    // Gradient (dT/dt) state variables
    float last_temp;
    uint32_t last_time_ms;
};