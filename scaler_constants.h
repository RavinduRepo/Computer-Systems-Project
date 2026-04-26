// Auto-generated — do not edit
#ifndef SCALER_CONSTANTS_H
#define SCALER_CONSTANTS_H

const int N_FEATURES = 5;
const int N_CLASSES  = 3;

const char* CLASS_LABELS[] = {"normal", "leak", "sensor_fault"};

const float FEATURE_MEAN[]  = {184.3253f, 1.3446f, 1.0000f, -0.1618f, 0.4949f};
const float FEATURE_SCALE[] = {47.7382f, 1.2021f, 1.0000f, 1.1994f, 0.8900f};

inline void normalise(float* x) {
  for (int i = 0; i < N_FEATURES; i++) {
    x[i] = (x[i] - FEATURE_MEAN[i]) / FEATURE_SCALE[i];
  }
}

inline int argmax(float* arr, int n) {
  int idx = 0;
  for (int i = 1; i < n; i++) {
    if (arr[i] > arr[idx]) idx = i;
  }
  return idx;
}

#endif
