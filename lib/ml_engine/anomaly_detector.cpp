// #include "anomaly_detector.h"
// #include <Arduino.h>
// #include <math.h>

// bool AnomalyDetector::begin() {
//     Serial.println("[STUB-ML] Mock AnomalyDetector initialized (Static 4KB arena bypassed)");
//     return true;
// }

// bool AnomalyDetector::evaluateConditions(
//     float temp_c, 
//     float temp_gradient, 
//     float humidity, 
//     float vib_rms, 
//     float dew_point, 
//     MLInference &out_result
// ) {
//     // Normal operation mock: Nominal baseline between 2°C and 8°C[cite: 1]
//     // Simulates low MSE during nominal conditions, elevated MSE during spikes
//     if (temp_c < 2.0f || temp_c > 8.0f || fabsf(temp_gradient) > 1.5f || vib_rms > 2.0f) {
//         out_result.reconstruction_mse = 0.062f; // Exceeds 0.045 threshold[cite: 1]
//         out_result.anomaly_flag = true;         //[cite: 1]
//     } else {
//         out_result.reconstruction_mse = 0.015f; // Nominal baseline[cite: 1]
//         out_result.anomaly_flag = false;        //[cite: 1]
//     }

//     Serial.printf("[STUB-ML] Ingested T:%.2f Grad:%.2f RH:%.2f -> MSE:%.4f (Anomaly:%s)\n",
//                   temp_c, temp_gradient, humidity, 
//                   out_result.reconstruction_mse, 
//                   out_result.anomaly_flag ? "TRUE" : "FALSE");

//     return true;
// }
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


// ============================================================
// NORMALIZATION PARAMETERS
// These are exactly the parameters saved from Python
// ============================================================

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


// ============================================================
// INT8 QUANTIZATION PARAMETERS
//
// Obtained from your TFLite model:
//
// INPUT:
// scale      = 0.033087629824876785
// zero_point = -8
//
// OUTPUT:
// scale      = 0.03310078755021095
// zero_point = -8
// ============================================================

constexpr float INPUT_SCALE = 0.033087629824876785f;
constexpr int INPUT_ZERO_POINT = -8;

constexpr float OUTPUT_SCALE = 0.03310078755021095f;
constexpr int OUTPUT_ZERO_POINT = -8;


// ============================================================
// TENSOR ARENA
// ============================================================
//
// 16 KB for initial testing.
// If AllocateTensors() fails, we can increase this later.
// ============================================================

constexpr int TENSOR_ARENA_SIZE = 16 * 1024;

static uint8_t tensor_arena[TENSOR_ARENA_SIZE];


// ============================================================
// TFLITE MICRO OBJECTS
// ============================================================

namespace {

tflite::ErrorReporter* error_reporter = nullptr;

const tflite::Model* model = nullptr;

tflite::MicroInterpreter* interpreter = nullptr;

TfLiteTensor* input_tensor = nullptr;

TfLiteTensor* output_tensor = nullptr;

} // namespace


// ============================================================
// BEGIN
// ============================================================

bool AnomalyDetector::begin()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("     TFLite Micro Anomaly Detector");
    Serial.println("========================================");


    // --------------------------------------------------------
    // Error reporter
    // --------------------------------------------------------

    static tflite::MicroErrorReporter micro_error_reporter;

    error_reporter = &micro_error_reporter;

    Serial.println("[ML] Error reporter initialized");


    // --------------------------------------------------------
    // Load TFLite model
    // --------------------------------------------------------

    model = tflite::GetModel(
        cold_chain_autoencoder_int8_tflite
    );

    if (model == nullptr) {

        Serial.println(
            "[ML] ERROR: Model is NULL"
        );

        return false;
    }

    Serial.println(
        "[ML] Model loaded successfully"
    );


    // --------------------------------------------------------
    // Check TFLite schema version
    // --------------------------------------------------------

    if (model->version() != TFLITE_SCHEMA_VERSION) {

        Serial.printf(
            "[ML] ERROR: Schema mismatch. "
            "Model=%d Supported=%d\n",
            model->version(),
            TFLITE_SCHEMA_VERSION
        );

        return false;
    }

    Serial.printf(
        "[ML] Schema version: %d\n",
        model->version()
    );


    // --------------------------------------------------------
    // Register model operations
    //
    // Your Keras model:
    //
    // Dense(16, relu)
    // Dense(8, relu)
    // Dense(16, relu)
    // Dense(5, linear)
    //
    // Required operations:
    //
    // FullyConnected
    // Relu
    // --------------------------------------------------------

    static tflite::MicroMutableOpResolver<2> resolver;


    if (resolver.AddFullyConnected() != kTfLiteOk) {

        Serial.println(
            "[ML] ERROR: Could not add FullyConnected"
        );

        return false;
    }


    if (resolver.AddRelu() != kTfLiteOk) {

        Serial.println(
            "[ML] ERROR: Could not add Relu"
        );

        return false;
    }


    Serial.println(
        "[ML] Operators registered"
    );


    // --------------------------------------------------------
    // Create TFLite Micro interpreter
    //
    // IMPORTANT:
    // This version of TensorFlowLite_ESP32 requires
    // error_reporter as the 5th argument.
    // --------------------------------------------------------

    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE,
        error_reporter
    );

    interpreter = &static_interpreter;


    Serial.println(
        "[ML] Interpreter created"
    );


    // --------------------------------------------------------
    // Allocate tensors
    // --------------------------------------------------------

    TfLiteStatus allocate_status =
        interpreter->AllocateTensors();

    if (allocate_status != kTfLiteOk) {

        Serial.println(
            "[ML] ERROR: AllocateTensors() failed"
        );

        return false;
    }

    Serial.println(
        "[ML] Tensor allocation successful"
    );


    // --------------------------------------------------------
    // Get input tensor
    // --------------------------------------------------------

    input_tensor = interpreter->input(0);

    if (input_tensor == nullptr) {

        Serial.println(
            "[ML] ERROR: Input tensor is NULL"
        );

        return false;
    }


    // --------------------------------------------------------
    // Get output tensor
    // --------------------------------------------------------

    output_tensor = interpreter->output(0);

    if (output_tensor == nullptr) {

        Serial.println(
            "[ML] ERROR: Output tensor is NULL"
        );

        return false;
    }


    // --------------------------------------------------------
    // Print tensor information
    // --------------------------------------------------------

    Serial.println();
    Serial.println("----------- MODEL INFO -----------");


    Serial.printf(
        "Input type       : %d\n",
        input_tensor->type
    );

    Serial.printf(
        "Input scale      : %.10f\n",
        input_tensor->params.scale
    );

    Serial.printf(
        "Input zero point : %d\n",
        input_tensor->params.zero_point
    );


    Serial.printf(
        "Output type      : %d\n",
        output_tensor->type
    );

    Serial.printf(
        "Output scale     : %.10f\n",
        output_tensor->params.scale
    );

    Serial.printf(
        "Output zero point: %d\n",
        output_tensor->params.zero_point
    );


    Serial.println(
        "---------------------------------"
    );


    // --------------------------------------------------------
    // Verify INT8 model
    // --------------------------------------------------------

    if (input_tensor->type != kTfLiteInt8) {

        Serial.println(
            "[ML] WARNING: Input is NOT INT8"
        );

    } else {

        Serial.println(
            "[ML] Input INT8 confirmed"
        );
    }


    if (output_tensor->type != kTfLiteInt8) {

        Serial.println(
            "[ML] WARNING: Output is NOT INT8"
        );

    } else {

        Serial.println(
            "[ML] Output INT8 confirmed"
        );
    }


    Serial.println();
    Serial.println(
        "[ML] AnomalyDetector initialized successfully"
    );

    Serial.println(
        "========================================"
    );

    return true;
}


// ============================================================
// EVALUATE CONDITIONS
// ============================================================

bool AnomalyDetector::evaluateConditions(
    float temp_c,
    float temp_gradient,
    float humidity,
    float vib_rms,
    float dew_point,
    MLInference &out_result
)
{

    // --------------------------------------------------------
    // Check initialization
    // --------------------------------------------------------

    if (interpreter == nullptr ||
        input_tensor == nullptr ||
        output_tensor == nullptr) {

        Serial.println(
            "[ML] ERROR: Detector not initialized"
        );

        return false;
    }


    // ========================================================
    // 1. RAW INPUT
    // ========================================================

    float raw_input[5] = {

        temp_c,

        temp_gradient,

        humidity,

        vib_rms,

        dew_point
    };


    // ========================================================
    // 2. NORMALIZATION
    //
    // Python:
    //
    // X_norm = (X - mean) / std
    //
    // Exactly the same calculation is performed here.
    // ========================================================

    float normalized[5];


    for (int i = 0; i < 5; i++) {

        normalized[i] =
            (raw_input[i] - FEATURE_MEAN[i])
            / FEATURE_STD[i];
    }


    // ========================================================
    // 3. FLOAT → INT8
    //
    // Quantization:
    //
    // q = round(float_value / scale)
    //     + zero_point
    // ========================================================

    for (int i = 0; i < 5; i++) {

        int32_t quantized_value =
            (int32_t)roundf(
                normalized[i] / INPUT_SCALE
            )
            + INPUT_ZERO_POINT;


        // ----------------------------------------------------
        // Saturate to INT8 range
        // ----------------------------------------------------

        if (quantized_value > 127) {

            quantized_value = 127;

        }

        if (quantized_value < -128) {

            quantized_value = -128;
        }


        input_tensor->data.int8[i] =
            (int8_t)quantized_value;
    }


    // ========================================================
    // 4. RUN NEURAL NETWORK
    // ========================================================

    TfLiteStatus status =
        interpreter->Invoke();


    if (status != kTfLiteOk) {

        Serial.println(
            "[ML] ERROR: Inference failed"
        );

        return false;
    }


    // ========================================================
    // 5. READ OUTPUT
    //
    // INT8 → FLOAT
    //
    // float = (q - zero_point) * scale
    // ========================================================

    float reconstructed[5];


    for (int i = 0; i < 5; i++) {

        int8_t quantized_output =
            output_tensor->data.int8[i];


        reconstructed[i] =
            (
                (float)quantized_output
                - OUTPUT_ZERO_POINT
            )
            * OUTPUT_SCALE;
    }


    // ========================================================
    // 6. CALCULATE RECONSTRUCTION MSE
    //
    // MSE is calculated in normalized space.
    // ========================================================

    float mse = 0.0f;


    for (int i = 0; i < 5; i++) {

        float error =
            normalized[i]
            - reconstructed[i];


        mse +=
            error * error;
    }


    mse /= 5.0f;


    // ========================================================
    // 7. STORE RESULT
    //
    // IMPORTANT:
    //
    // We are NOT using the anomaly threshold yet.
    //
    // Your current goal is simply:
    //
    // Sensor input
    //      ↓
    // Neural network
    //      ↓
    // Reconstructed output
    //
    // Accuracy/threshold will be handled later.
    // ========================================================

    out_result.reconstruction_mse = mse;

    out_result.anomaly_flag = (mse > THRESHOLD_ML_MSE);


    // ========================================================
    // 8. PRINT INPUT
    // ========================================================

    Serial.println();
    Serial.println(
        "=========== ML INFERENCE ==========="
    );


    Serial.println(
        "RAW INPUT:"
    );


    Serial.printf(
        "Temperature      : %.6f\n",
        temp_c
    );

    Serial.printf(
        "Temp Gradient    : %.6f\n",
        temp_gradient
    );

    Serial.printf(
        "Humidity         : %.6f\n",
        humidity
    );

    Serial.printf(
        "Vibration RMS    : %.6f\n",
        vib_rms
    );

    Serial.printf(
        "Dew Point        : %.6f\n",
        dew_point
    );


    // ========================================================
    // 9. PRINT NORMALIZED INPUT
    // ========================================================

    Serial.println();

    Serial.println(
        "NORMALIZED INPUT:"
    );


    Serial.printf(
        "%.6f, %.6f, %.6f, %.6f, %.6f\n",
        normalized[0],
        normalized[1],
        normalized[2],
        normalized[3],
        normalized[4]
    );


    // ========================================================
    // 10. PRINT INT8 INPUT
    // ========================================================

    Serial.println();

    Serial.println(
        "INT8 INPUT:"
    );


    for (int i = 0; i < 5; i++) {

        Serial.printf(
            "%d ",
            input_tensor->data.int8[i]
        );
    }

    Serial.println();


    // ========================================================
    // 11. PRINT RECONSTRUCTED OUTPUT
    // ========================================================

    Serial.println();

    Serial.println(
        "RECONSTRUCTED OUTPUT:"
    );


    Serial.printf(
        "%.6f, %.6f, %.6f, %.6f, %.6f\n",
        reconstructed[0],
        reconstructed[1],
        reconstructed[2],
        reconstructed[3],
        reconstructed[4]
    );


    // ========================================================
    // 12. PRINT INT8 OUTPUT
    // ========================================================

    Serial.println();

    Serial.println(
        "INT8 OUTPUT:"
    );


    for (int i = 0; i < 5; i++) {

        Serial.printf(
            "%d ",
            output_tensor->data.int8[i]
        );
    }

    Serial.println();


    // ========================================================
    // 13. PRINT MSE
    // ========================================================

    Serial.println();

    Serial.printf(
        "Reconstruction MSE: %.10f\n",
        mse
    );


    Serial.println(
        "===================================="
    );


    return true;
}