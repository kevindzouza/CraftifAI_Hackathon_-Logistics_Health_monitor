#ifndef HEALTH_SCORE_H
#define HEALTH_SCORE_H

#include "health_monitor.h"
#include <stdint.h>

typedef struct
{
  int32_t live_score;
  int32_t trip_score;
  uint32_t moderate_shocks;
  uint32_t severe_shocks;
  uint32_t vibration_events;
  uint32_t tilt_events;
  uint32_t temperature_events;
  uint32_t last_update_ms;
} health_score_t;

void health_score_init(health_score_t *score, uint32_t now_ms);
void health_score_reset(health_score_t *score, uint32_t now_ms);
void health_score_update(health_score_t *score, const health_state_t *state,
                        uint32_t now_ms, uint32_t elapsed_ms);

#endif
