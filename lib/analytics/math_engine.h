#pragma once
#include <stdint.h>
#include <math.h>

class MathEngine
{
public:
    MathEngine();

    float calculateDewPoint(float temp_c, float humidity_pct);
    float updateGradient(float current_temp_c, uint32_t current_time_ms);
    float updateMKT(float current_temp_c, uint32_t current_time_ms);
    float updateVibrationDose(float rms_g, float dt_seconds);

    void reset();

private:
    double weighted_exp_sum; // Weighted Arrhenius sum (double precision)
    double total_time_s;     // Total accumulated duration in seconds
    float last_temp;
    uint32_t last_time_ms;
    float cumulative_vib_dose;
};