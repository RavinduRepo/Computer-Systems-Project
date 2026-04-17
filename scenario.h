// This can be used to simulate different situations as a script without manual interventions
// scenario.h
#ifndef SCENARIO_H
#define SCENARIO_H

typedef enum {
  ACT_USE_SW,
  ACT_USE_RATE,
  ACT_LEAK_SW,
  ACT_LEAK_RATE
} action_type_t;

typedef struct {
  uint32_t time_ms;       // When the event happens (in milliseconds from boot)
  action_type_t action;   // What is changing
  uint32_t value;         // The new value (for rates, remember 10000 = 1.0 cm/s)
} sim_event_t;

// The simulation processes these in order as time passes.
sim_event_t scenario[] = {
  // At 2 seconds: Turn on water usage at 2.5 cm/s
  { 2000, ACT_USE_RATE, 25000 }, 
  { 2000, ACT_USE_SW, 1 },

  // At 10 seconds: Turn off water usage
  { 10000, ACT_USE_SW, 0 },

  // At 15 seconds: The tank springs a leak! (0.5 cm/s)
  { 15000, ACT_LEAK_RATE, 5000 },
  { 15000, ACT_LEAK_SW, 1 },

  // At 30 seconds: The leak is fixed
  { 30000, ACT_LEAK_SW, 0 }
};

const int scenario_length = sizeof(scenario) / sizeof(scenario[0]);

#endif
