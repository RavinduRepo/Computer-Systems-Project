#include <Arduino.h>

/* ════════════════════════════════════════════════════
 * MODE SELECTOR
 * ════════════════════════════════════════════════════ */
// #define MODE_DATA_COLLECTION
#define MODE_INFERENCE

/* ── Scenario selection (FIXED) ───────────────────── */
#ifdef MODE_DATA_COLLECTION
#include <scenario.h>
#endif

#ifdef MODE_INFERENCE
#include <scenario_inference.h>
#endif

#include "water_fault_model.h"
#include "scaler_constants.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

/* ── Pins ─────────────────────────────────────────── */
const int trigPin     = 5;
const int echoPin     = 18;
const int waterUsePin = 6;

/* ── SENSOR TIMING ────────────────────────────────── */
#define SENSOR_READ_INTERVAL 1000

/* ── Physics ─────────────────────────────────────── */
const float USAGE_DRAIN_RATE = 1.5f;
const float IDLE_BASELINE    = 0.1f;

/* ── Rolling buffer ─────────────────────────────── */
#define VAR_WINDOW_SECONDS 4
#define BUFFER_SIZE (VAR_WINDOW_SECONDS * 1000 / SENSOR_READ_INTERVAL)

float delta_buf[BUFFER_SIZE] = {0};
int delta_buf_idx = 0;

unsigned long last_read_time = 0;
float last_distance = 120.0f;

int water_in_use = 1;
int warmup = 5;

/* ── TFLite Setup ───────────────────────────────── */
constexpr int TENSOR_ARENA_SIZE = 10 * 1024;
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

tflite::MicroInterpreter* interpreter;
TfLiteTensor* input;
TfLiteTensor* output;

/* ── Ultrasonic ─────────────────────────────────── */
float read_distance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return last_distance;

  float d = duration * 0.034f / 2.0f;
  d += random(-2, 2) * 0.05f;

  if (d < 5.0f || d > 250.0f) return last_distance;
  return d;
}

float expected_delta(int in_use, float dt_sec) {
  float base = in_use ? USAGE_DRAIN_RATE : IDLE_BASELINE;
  return base * dt_sec;
}

float compute_variance(float *buf, int size) {
  float mean = 0;
  for (int i = 0; i < size; i++) mean += buf[i];
  mean /= size;

  float v = 0;
  for (int i = 0; i < size; i++) {
    float diff = buf[i] - mean;
    v += diff * diff;
  }
  return v / size;
}

/* ── Scenario label (ONLY for data collection) ───── */
#ifdef MODE_DATA_COLLECTION
int get_scenario_label(unsigned long t) {
  for (int i = scenario_timing_length - 1; i >= 0; i--) {
    if (t >= scenario_timing[i].start_time_ms) {
      return scenario_timing[i].label;
    }
  }
  return -1;
}
#endif

/* ── Setup ───────────────────────────────────────── */
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(waterUsePin, INPUT);

#ifdef MODE_DATA_COLLECTION
  Serial.println("timestamp,distance,delta,in_use,residual,var,label");
#endif

#ifdef MODE_INFERENCE
  Serial.println("TFLite Inference Mode");

  const tflite::Model* model = tflite::GetModel(water_fault_model);

  static tflite::AllOpsResolver resolver;

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, TENSOR_ARENA_SIZE);

  interpreter = &static_interpreter;

  interpreter->AllocateTensors();

  input  = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Model loaded");
#endif
}

/* ── Loop ────────────────────────────────────────── */
void loop() {
  unsigned long now = millis();

  if (now - last_read_time < SENSOR_READ_INTERVAL) {
    delay(10);
    return;
  }

  float dt_sec = (now - last_read_time) / 1000.0f;
  last_read_time = now;

  float distance = read_distance();
  water_in_use = 1;

  if (warmup > 0) {
    warmup--;
    last_distance = distance;
    return;
  }

  float delta = (distance - last_distance);

  if (abs(delta) < 0.0001f) {
    delta += random(-1,1) * 0.002f;
  }

  delta_buf[delta_buf_idx] = delta;
  delta_buf_idx = (delta_buf_idx + 1) % BUFFER_SIZE;

  float residual = delta - expected_delta(water_in_use, dt_sec);
  float var = compute_variance(delta_buf, BUFFER_SIZE);

  last_distance = distance;

#ifdef MODE_DATA_COLLECTION
  int label = get_scenario_label(now);
  if (label == -1) return;

  Serial.print(now); Serial.print(",");
  Serial.print(distance, 2); Serial.print(",");
  Serial.print(delta, 3); Serial.print(",");
  Serial.print(water_in_use); Serial.print(",");
  Serial.print(residual, 3); Serial.print(",");
  Serial.print(var, 4); Serial.print(",");
  Serial.println(label);
#endif

#ifdef MODE_INFERENCE

  float features[N_FEATURES] = {
    distance,
    delta,
    (float)water_in_use,
    residual,
    var
  };

  normalise_features(features);

  for (int i = 0; i < N_FEATURES; i++) {
    input->data.f[i] = features[i];
  }

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Inference failed!");
    return;
  }

  int pred = argmax(output->data.f, N_CLASSES);
  float conf = output->data.f[pred];

  Serial.print(now);
  Serial.print(" | d="); Serial.print(distance, 1);
  Serial.print(" | Δ="); Serial.print(delta, 2);
  Serial.print(" | res="); Serial.print(residual, 2);
  Serial.print(" | var="); Serial.print(var, 3);
  Serial.print(" | pred="); Serial.print(CLASS_LABELS[pred]);
  Serial.print(" ("); Serial.print(conf, 2); Serial.println(")");

#endif
}
