// ═══════════════════════════════════════════════════════
// INFERENCE TEST SCENARIO (3-CLASS)
// ═══════════════════════════════════════════════════════

#ifndef SCENARIO_H
#define SCENARIO_H

#include <cstdint>

typedef enum {
  ACT_USE_SW,
  ACT_USE_RATE,
  ACT_LEAK_SW,
  ACT_LEAK_RATE
} action_type_t;

typedef struct {
  uint32_t time_ms;
  action_type_t action;
  uint32_t value;
} sim_event_t;

/* ── TEST SCENARIO ───────────────────────────────── */

sim_event_t scenario[] = {

  /* INIT */
  { 500, ACT_USE_SW, 1 },
  { 500, ACT_USE_RATE, 5000 },
  { 500, ACT_LEAK_SW, 0 },

  /* ───────── NORMAL ───────── */
  { 1000, ACT_USE_SW, 1 },

  /* steady usage */
  { 10000, ACT_USE_RATE, 5000 },

  /* ───────── LEAK ───────── */
  { 20000, ACT_LEAK_SW, 1 },
  { 20000, ACT_LEAK_RATE, 15000 },

  { 35000, ACT_LEAK_SW, 0 },

  /* ───────── SENSOR FAULT ───────── */
  { 40000, ACT_LEAK_SW, 1 },
  { 40000, ACT_LEAK_RATE, 40000 },

  /* oscillation */
  { 42000, ACT_LEAK_SW, 0 }, { 44000, ACT_LEAK_SW, 1 },
  { 46000, ACT_LEAK_SW, 0 }, { 48000, ACT_LEAK_SW, 1 },
  { 50000, ACT_LEAK_SW, 0 }, { 52000, ACT_LEAK_SW, 1 },
  { 54000, ACT_LEAK_SW, 0 },

  /* END */
  { 60000, ACT_LEAK_SW, 0 }
};

const int scenario_length = sizeof(scenario) / sizeof(scenario[0]);

#endif
