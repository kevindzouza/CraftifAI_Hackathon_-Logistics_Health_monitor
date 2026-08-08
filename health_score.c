#include "health_score.h"
#include "app_config.h"

#define SCORE_START                  100
#define MODERATE_SHOCK_PENALTY       5
#define SEVERE_SHOCK_PENALTY         15
#define VIBRATION_PENALTY_INTERVAL   10000U
#define VIBRATION_PENALTY            1
#define TILT_PENALTY_INTERVAL        5000U
#define TILT_ZERO_SCORE_INTERVAL     10000U
#define TILT_PENALTY                 APP_TILT_SCORE_PENALTY
#define TEMP_PENALTY_INTERVAL        60000U
#define TEMP_PENALTY                 APP_TEMP_SCORE_PENALTY

static uint8_t previous_shock;
static uint8_t previous_vibration;
static uint8_t previous_tilt;
static uint8_t previous_temperature;
static uint32_t vibration_credit_ms;
static uint32_t tilt_credit_ms;
static uint32_t tilt_active_ms;
static uint32_t temperature_credit_ms;

static void deduct(health_score_t *score, int32_t points)
{
  score->live_score -= points;
  if (score->live_score < 0) score->live_score = 0;
  score->trip_score = score->live_score;
}

void health_score_init(health_score_t *score, uint32_t now_ms)
{
  *score = (health_score_t){.live_score = SCORE_START, .trip_score = SCORE_START,
                            .last_update_ms = now_ms};
  previous_shock = 0U;
  previous_vibration = 0U;
  previous_tilt = 0U;
  previous_temperature = 0U;
  vibration_credit_ms = 0U;
  tilt_credit_ms = 0U;
  tilt_active_ms = 0U;
  temperature_credit_ms = 0U;
}

void health_score_reset(health_score_t *score, uint32_t now_ms)
{
  score->live_score = SCORE_START;
  score->trip_score = SCORE_START;
  score->moderate_shocks = 0U;
  score->severe_shocks = 0U;
  score->vibration_events = 0U;
  score->tilt_events = 0U;
  score->temperature_events = 0U;
  score->last_update_ms = now_ms;
  previous_shock = 0U;
  previous_vibration = 0U;
  previous_tilt = 0U;
  previous_temperature = 0U;
  vibration_credit_ms = 0U;
  tilt_credit_ms = 0U;
  tilt_active_ms = 0U;
  temperature_credit_ms = 0U;
}

void health_score_update(health_score_t *score, const health_state_t *state,
                        uint32_t now_ms, uint32_t elapsed_ms)
{
  if (state->shock_event != 0U && previous_shock == 0U)
  {
    if (state->shock_level >= 2U)
    {
      score->severe_shocks++;
      deduct(score, SEVERE_SHOCK_PENALTY);
    }
    else
    {
      score->moderate_shocks++;
      deduct(score, MODERATE_SHOCK_PENALTY);
    }
  }
  if (state->vibration_event != 0U)
  {
    score->vibration_events++;
    deduct(score, VIBRATION_PENALTY);
  }
  if (state->vibration_event != 0U)
  {
    vibration_credit_ms += elapsed_ms;
    if (vibration_credit_ms >= VIBRATION_PENALTY_INTERVAL)
    {
      vibration_credit_ms -= VIBRATION_PENALTY_INTERVAL;
      deduct(score, VIBRATION_PENALTY);
    }
  }
  if (state->improper_tilt != 0U && previous_tilt == 0U)
  {
    score->tilt_events++;
    deduct(score, TILT_PENALTY);
  }
  if (state->improper_tilt != 0U)
  {
    tilt_active_ms += elapsed_ms;
    tilt_credit_ms += elapsed_ms;
    if (tilt_credit_ms >= TILT_PENALTY_INTERVAL)
    {
      tilt_credit_ms -= TILT_PENALTY_INTERVAL;
      deduct(score, TILT_PENALTY);
    }
    if (tilt_active_ms >= TILT_ZERO_SCORE_INTERVAL)
    {
      score->live_score = 0;
      score->trip_score = 0;
    }
  }
  else
  {
    tilt_active_ms = 0U;
    tilt_credit_ms = 0U;
  }
  if (state->temperature_event != 0U)
  {
    score->temperature_events++;
    deduct(score, TEMP_PENALTY);
  }
  if (state->temperature_event != 0U)
  {
    temperature_credit_ms += elapsed_ms;
    if (temperature_credit_ms >= TEMP_PENALTY_INTERVAL)
    {
      temperature_credit_ms -= TEMP_PENALTY_INTERVAL;
      deduct(score, TEMP_PENALTY);
    }
  }
  previous_shock = state->shock_level;
  previous_vibration = state->vibration_excessive;
  previous_tilt = state->improper_tilt;
  previous_temperature = state->temperature_excursion;
  score->last_update_ms = now_ms;
}
