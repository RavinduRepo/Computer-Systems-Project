#ifndef SCENARIO_H
#define SCENARIO_H
#include <stdint.h>

typedef enum {
  ACT_USE_SW,
  ACT_USE_RATE,
  ACT_LEAK_SW,
  ACT_LEAK_RATE
} action_type_t;

typedef struct {
  uint32_t      time_ms;
  action_type_t action;
  uint32_t      value;
} sim_event_t;

typedef struct {
  uint32_t start_time_ms;
  int      label;
} scenario_timing_t;

/* ── SCENARIO EVENTS ───────────────────────────── */
sim_event_t scenario[] = {

  /* INIT */
  {  500, ACT_USE_SW,   1 },
  {  500, ACT_USE_RATE, 5000 },
  {  500, ACT_LEAK_SW,  0 },
  {  500, ACT_LEAK_RATE,0 },

  /* ───────── CLASS 0: NORMAL ───────── */
  { 1000, ACT_USE_SW,   1 },

  /* ───────── RESET → PREPARE LEAK ───────── */
  { 25000, ACT_LEAK_SW, 1 },
  { 25000, ACT_LEAK_RATE,10000 },

  /* ───────── CLASS 1: LEAK ───────── */
  { 40000, ACT_LEAK_SW, 1 },
  { 40000, ACT_LEAK_RATE,15000 },

  /* 🔧 FIX: STOP LEAK BEFORE EMPTY */
  { 60000, ACT_LEAK_SW, 0 },

  /* ───────── RESET BEFORE FAULT ───────── */
  { 65000, ACT_LEAK_SW, 1 },
  { 65000, ACT_LEAK_RATE,10000 },

  /* ───────── CLASS 2: SENSOR FAULT ───────── */
  /* 🔧 FIX: MOVED EARLIER */
  { 75000, ACT_LEAK_SW, 1 },
  { 75000, ACT_LEAK_RATE,40000 },

  /* oscillation pattern */
  { 77000, ACT_LEAK_SW, 0 }, { 79000, ACT_LEAK_SW, 1 },
  { 81000, ACT_LEAK_SW, 0 }, { 83000, ACT_LEAK_SW, 1 },
  { 85000, ACT_LEAK_SW, 0 }, { 87000, ACT_LEAK_SW, 1 },
  { 89000, ACT_LEAK_SW, 0 }, { 91000, ACT_LEAK_SW, 1 },
  { 93000, ACT_LEAK_SW, 0 }, { 95000, ACT_LEAK_SW, 1 },
  { 97000, ACT_LEAK_SW, 0 }, { 99000, ACT_LEAK_SW, 1 },
};

const int scenario_length = sizeof(scenario) / sizeof(scenario[0]);

/* ── LABEL TIMING ───────────────────────────── */
const scenario_timing_t scenario_timing[] = {
  {     0, -1 },
  {  1000,  0 },   /* Normal */
  { 25000, -1 },
  { 40000,  1 },   /* Leak */
  { 60000, -1 },
  { 75000,  2 },   /* Sensor Fault */
  {100000, -1 },
};

const int scenario_timing_length =
  sizeof(scenario_timing) / sizeof(scenario_timing[0]);

#endif
