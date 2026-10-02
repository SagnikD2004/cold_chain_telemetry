#include "math_engine.h"

#define DELTA_H_OVER_R 10000.0f 

MathEngine::MathEngine() : exp_sum(0.0), sample_count(0), last_temp(-999.0f), last_time_ms(0) {}

float MathEngine::calculateDewPoint(float temp_c, float humidity_pct) {
    if (humidity_pct <= 0.0f) return temp_c;
    
    float alpha = ((17.625f * temp_c) / (243.04f + temp_c)) + log(humidity_pct / 100.0f);
    return (243.04f * alpha) / (17.625f - alpha);
}

float MathEngine::updateGradient(float current_temp_c, uint32_t current_time_ms) {
    if (last_temp == -999.0f) {
        last_temp = current_temp_c;
        last_time_ms = current_time_ms;
        return 0.0f; 
    }
    
    float dt_min = (current_time_ms - last_time_ms) / 60000.0f;
    float gradient = 0.0f;
    
    if (dt_min > 0) {
        gradient = (current_temp_c - last_temp) / dt_min;
    }
    
    last_temp = current_temp_c;
    last_time_ms = current_time_ms;
    
    return gradient; 
}

float MathEngine::updateMKT(float current_temp_c) {
    float temp_k = current_temp_c + 273.15f;
    
    exp_sum += exp(-DELTA_H_OVER_R / temp_k);
    sample_count++;
    
    float avg_exp = exp_sum / (float)sample_count;
    float mkt_k = -DELTA_H_OVER_R / log(avg_exp);
    
    return mkt_k - 273.15f; 
}