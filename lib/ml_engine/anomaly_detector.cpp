#include "anomaly_detector.h"
#include "model_data.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

#include <TensorFlowLite_ESP32.h>

#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const float FEATURE_MEAN[5] = {
    32.499092f,
    0.0004941501f,
    68.003632f,
    0.92549688f,
    25.825844f
};

static const float FEATURE_STD[5] = {
    1.4450436f,
    0.05653532f,
    1.7438841f,
    0.33269802f,
    1.8063866f
};

constexpr float INPUT_SCALE = 0.033087629824876785f;
constexpr int INPUT_ZERO_POINT = -8;

constexpr float OUTPUT_SCALE = 0.03310078755021095f;
constexpr int OUTPUT_ZERO_POINT = -8;

constexpr int TENSOR_ARENA_SIZE = 16 * 1024;

static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input_tensor = nullptr;
    TfLiteTensor *output_tensor = nullptr;
}

bool AnomalyDetector::begin() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("     TFLite Micro Anomaly Detector");
    Serial.println("========================================");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    Serial.println("[ML] Error reporter initialized");

    model = tflite::GetModel(cold_chain_autoencoder_int8_tflite);

    if (model == nullptr) {
        Serial.println("[ML] ERROR: Model is NULL");
        return false;
    }

    Serial.println("[ML] Model loaded successfully");

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf(
            "[ML] ERROR: Schema mismatch. Model=%d Supported=%d\n",
            model->version(),
            TFLITE_SCHEMA_VERSION
        );
        return false;
    }

    Serial.printf("[ML] Schema version: %d\n", model->version());

    static tflite::MicroMutableOpResolver<2> resolver;

    if (resolver.AddFullyConnected() != kTfLiteOk) {
        Serial.println("[ML] ERROR: Could not add FullyConnected");
        return false;
    }

    if (resolver.AddRelu() != kTfLiteOk) {
        Serial.println("[ML] ERROR: Could not add Relu");
        return false;
    }

    Serial.println("[ML] Operators registered");

    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE,
        error_reporter
    );

    interpreter = &static_interpreter;

    Serial.println("[ML] Interpreter created");

    TfLiteStatus allocate_status = interpreter->AllocateTensors();

    if (allocate_status != kTfLiteOk) {
        Serial.println("[ML] ERROR: AllocateTensors() failed");
        return false;
    }

    Serial.println("[ML] Tensor allocation successful");

    input_tensor = interpreter->input(0);

    if (input_tensor == nullptr) {
        Serial.println("[ML] ERROR: Input tensor is NULL");
        return false;
    }

    output_tensor = interpreter->output(0);

    if (output_tensor == nullptr) {
        Serial.println("[ML] ERROR: Output tensor is NULL");
        return false;
    }

    Serial.println();
    Serial.println("----------- MODEL INFO -----------");
    Serial.printf("Input type       : %d\n", input_tensor->type);
    Serial.printf("Input scale      : %.10f\n", input_tensor->params.scale);
    Serial.printf("Input zero point : %d\n", input_tensor->params.zero_point);
    Serial.printf("Output type      : %d\n", output_tensor->type);
    Serial.printf("Output scale     : %.10f\n", output_tensor->params.scale);
    Serial.printf("Output zero point: %d\n", output_tensor->params.zero_point);
    Serial.println("---------------------------------");

    if (input_tensor->type != kTfLiteInt8) {
        Serial.println("[ML] WARNING: Input is NOT INT8");
    } else {
        Serial.println("[ML] Input INT8 confirmed");
    }

    if (output_tensor->type != kTfLiteInt8) {
        Serial.println("[ML] WARNING: Output is NOT INT8");
    } else {
        Serial.println("[ML] Output INT8 confirmed");
    }

    Serial.println();
    Serial.println("[ML] AnomalyDetector initialized successfully");
    Serial.println("========================================");

    return true;
}

bool AnomalyDetector::evaluateConditions(
    float temp_c,
    float temp_gradient,
    float humidity,
    float vib_rms,
    float dew_point,
    MLInference &out_result
) {
    if (interpreter == nullptr || input_tensor == nullptr || output_tensor == nullptr) {
        Serial.println("[ML] ERROR: Detector not initialized");
        return false;
    }

    float raw_input[5] = {
        temp_c,
        temp_gradient,
        humidity,
        vib_rms,
        dew_point
    };

    float normalized[5];

    for (int i = 0; i < 5; i++) {
        normalized[i] = (raw_input[i] - FEATURE_MEAN[i]) / FEATURE_STD[i];
    }

    for (int i = 0; i < 5; i++) {
        int32_t quantized_value = (int32_t)roundf(normalized[i] / INPUT_SCALE) + INPUT_ZERO_POINT;

        if (quantized_value > 127) {
            quantized_value = 127;
        }

        if (quantized_value < -128) {
            quantized_value = -128;
        }

        input_tensor->data.int8[i] = (int8_t)quantized_value;
    }

    TfLiteStatus status = interpreter->Invoke();

    if (status != kTfLiteOk) {
        Serial.println("[ML] ERROR: Inference failed");
        return false;
    }

    float reconstructed[5];

    for (int i = 0; i < 5; i++) {
        int8_t quantized_output = output_tensor->data.int8[i];
        reconstructed[i] = ((float)quantized_output - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
    }

    float mse = 0.0f;

    for (int i = 0; i < 5; i++) {
        float error = normalized[i] - reconstructed[i];
        mse += error * error;
    }

    mse /= 5.0f;

    out_result.reconstruction_mse = mse;
    out_result.anomaly_flag = (mse > THRESHOLD_ML_MSE);

    Serial.println();
    Serial.println("=========== ML INFERENCE ===========");
    Serial.println("RAW INPUT:");
    Serial.printf("Temperature      : %.6f\n", temp_c);
    Serial.printf("Temp Gradient    : %.6f\n", temp_gradient);
    Serial.printf("Humidity         : %.6f\n", humidity);
    Serial.printf("Vibration RMS    : %.6f\n", vib_rms);
    Serial.printf("Dew Point        : %.6f\n", dew_point);

    Serial.println();
    Serial.println("NORMALIZED INPUT:");
    Serial.printf(
        "%.6f, %.6f, %.6f, %.6f, %.6f\n",
        normalized[0],
        normalized[1],
        normalized[2],
        normalized[3],
        normalized[4]
    );

    Serial.println();
    Serial.println("INT8 INPUT:");

    for (int i = 0; i < 5; i++) {
        Serial.printf("%d ", input_tensor->data.int8[i]);
    }

    Serial.println();

    Serial.println();
    Serial.println("RECONSTRUCTED OUTPUT:");
    Serial.printf(
        "%.6f, %.6f, %.6f, %.6f, %.6f\n",
        reconstructed[0],
        reconstructed[1],
        reconstructed[2],
        reconstructed[3],
        reconstructed[4]
    );

    Serial.println();
    Serial.println("INT8 OUTPUT:");

    for (int i = 0; i < 5; i++) {
        Serial.printf("%d ", output_tensor->data.int8[i]);
    }

    Serial.println();

    Serial.println();
    Serial.printf("Reconstruction MSE: %.10f\n", mse);
    Serial.println("====================================");

    return true;
}