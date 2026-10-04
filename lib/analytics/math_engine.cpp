#include "math_engine.h"
#include <math.h>

#define DELTA_H_OVER_R 10000.0   // Activation energy ratio in Kelvin

MathEngine::MathEngine() {
    reset();
}

void MathEngine::reset() {
    weighted_exp_sum = 0.0;
    total_time_s = 0.0;
    last_temp = -999.0f;
    last_time_ms = 0;
    cumulative_vib_dose = 0.0f;
}

float MathEngine::calculateDewPoint(float temp_c, float humidity_pct) {
    if (humidity_pct <= 0.0f) return temp_c;
    if (humidity_pct > 100.0f) humidity_pct = 100.0f;

    float alpha = ((17.625f * temp_c) / (243.04f + temp_c)) + logf(humidity_pct / 100.0f);
    return (243.04f * alpha) / (17.625f - alpha);
}

float MathEngine::updateGradient(float current_temp_c, uint32_t current_time_ms) {
    if (last_temp == -999.0f) {
        last_temp = current_temp_c;
        last_time_ms = current_time_ms;
        return 0.0f;
    }

    uint32_t dt_ms = current_time_ms - last_time_ms;
    if (dt_ms < 500) {
        return 0.0f; // Ignore high-frequency bounce / jitter
    }

    // float dt_min = (float)dt_ms / 60000.0f;
    float dt_seconds = (float)dt_ms / 1000.0f;
    float gradient = (current_temp_c - last_temp) / dt_seconds;

    last_temp = current_temp_c;
    last_time_ms = current_time_ms;

    return gradient;
}

float MathEngine::updateMKT(float current_temp_c, uint32_t current_time_ms) {
    double temp_k = (double)current_temp_c + 273.15;
    
    double dt_s = 1.0;
    if (last_time_ms > 0 && current_time_ms > last_time_ms) {
        dt_s = (double)(current_time_ms - last_time_ms) / 1000.0;
    }

    weighted_exp_sum += exp(-DELTA_H_OVER_R / temp_k) * dt_s;
    total_time_s += dt_s;

    if (total_time_s <= 0.0) return current_temp_c;

    double avg_exp = weighted_exp_sum / total_time_s;
    if (avg_exp <= 0.0) return current_temp_c;

    double mkt_k = DELTA_H_OVER_R / (-log(avg_exp));
    return (float)(mkt_k - 273.15);
}

float MathEngine::updateVibrationDose(float rms_g, float dt_seconds) {
    cumulative_vib_dose += powf(rms_g, 4.0f) * dt_seconds;
    return powf(cumulative_vib_dose, 0.25f);
}