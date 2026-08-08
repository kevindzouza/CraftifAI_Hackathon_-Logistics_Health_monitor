#include "health_monitor.h"
#include "app_config.h"
#include <math.h>

#define SHOCK_MODERATE_G       2.5f
#define SHOCK_SEVERE_G         4.0f
#define VIBRATION_G_LIMIT      0.08f
#define VIBRATION_DPS_LIMIT    8.0f
#define VIBRATION_WINDOW       40U /* 200 ms at 200 Hz; three windows = 600 ms */
#define MIN_ELEVATED_WINDOWS   3U
#define SHOCK_VIBRATION_BLANK_MS 400U
#define TILT_LIMIT_DEG         APP_TILT_LIMIT_DEG
#define TILT_CONFIRM_MS        APP_TILT_CONFIRM_MS
#define TEMP_MIN_C             APP_TEMP_MIN_C
#define TEMP_MAX_C             APP_TEMP_MAX_C
#define TEMP_CONFIRM_MS        APP_TEMP_CONFIRM_MS
#define COMPLEMENTARY_ALPHA    0.98f

static float gyro_sq[VIBRATION_WINDOW];
static float accel_hp_sq[VIBRATION_WINDOW];
static uint32_t window_index;
static uint32_t window_count;
static uint32_t window_start_ms;
static uint32_t vibration_blank_until;
static uint32_t consecutive_elevated;
static uint8_t vibration_active;
static float accel_lp[3];
static float pitch_deg;
static float roll_deg;
static uint32_t tilt_started;
static uint32_t temperature_started;

void health_monitor_init(health_state_t *state, uint32_t now_ms)
{
  *state = (health_state_t){0};
  window_index = 0U;
  window_count = 0U;
  window_start_ms = now_ms;
  vibration_blank_until = 0U;
  consecutive_elevated = 0U;
  vibration_active = 0U;
  accel_lp[0] = 0.0f;
  accel_lp[1] = 0.0f;
  accel_lp[2] = 1.0f;
  pitch_deg = 0.0f;
  roll_deg = 0.0f;
  tilt_started = 0U;
  temperature_started = 0U;
}

void health_monitor_process(health_state_t *state, const mpu6050_sample_t *sample,
                            uint32_t now_ms, uint32_t elapsed_ms)
{
  const float dt = (elapsed_ms > 0U) ? ((float)elapsed_ms / 1000.0f) : 0.005f;
  const float alpha = 0.90f;
  float ax = sample->accel_g[0];
  float ay = sample->accel_g[1];
  float az = sample->accel_g[2];
  float accel_mag = sqrtf(ax * ax + ay * ay + az * az);
  float accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2957795f;
  float accel_roll = atan2f(ay, az) * 57.2957795f;
  float gyro_mag = sqrtf(sample->gyro_dps[0] * sample->gyro_dps[0] +
                          sample->gyro_dps[1] * sample->gyro_dps[1] +
                          sample->gyro_dps[2] * sample->gyro_dps[2]);
  float hp_x;
  float hp_y;
  float hp_z;
  float sum_gyro = 0.0f;
  float sum_accel = 0.0f;
  uint8_t window_ready = 0U;
  uint32_t i;

  state->shock_event = 0U;
  state->vibration_event = 0U;
  state->acceleration_magnitude_g = accel_mag;
  state->shock_level = (accel_mag >= SHOCK_SEVERE_G) ? 2U :
                       (accel_mag >= SHOCK_MODERATE_G) ? 1U : 0U;
  if (state->shock_level != 0U)
  {
    state->shock_event = 1U;
    vibration_blank_until = now_ms + SHOCK_VIBRATION_BLANK_MS;
    window_index = 0U;
    window_count = 0U;
    consecutive_elevated = 0U;
    vibration_active = 0U;
  }

  accel_lp[0] = alpha * accel_lp[0] + (1.0f - alpha) * ax;
  accel_lp[1] = alpha * accel_lp[1] + (1.0f - alpha) * ay;
  accel_lp[2] = alpha * accel_lp[2] + (1.0f - alpha) * az;
  hp_x = ax - accel_lp[0];
  hp_y = ay - accel_lp[1];
  hp_z = az - accel_lp[2];

  if ((int32_t)(now_ms - vibration_blank_until) >= 0)
  {
    gyro_sq[window_index] = gyro_mag * gyro_mag;
    accel_hp_sq[window_index] = hp_x * hp_x + hp_y * hp_y + hp_z * hp_z;
    window_index = (window_index + 1U) % VIBRATION_WINDOW;
    if (window_count < VIBRATION_WINDOW) window_count++;
    if (window_count == VIBRATION_WINDOW) window_ready = 1U;
  }

  if (window_ready != 0U)
  {
    for (i = 0U; i < VIBRATION_WINDOW; ++i)
    {
      sum_gyro += gyro_sq[i];
      sum_accel += accel_hp_sq[i];
    }
    state->gyro_rms_dps = sqrtf(sum_gyro / (float)VIBRATION_WINDOW);
    state->accel_vibration_rms_g = sqrtf(sum_accel / (float)VIBRATION_WINDOW);
    state->vibration_excessive = (state->accel_vibration_rms_g >= VIBRATION_G_LIMIT ||
                                  state->gyro_rms_dps >= VIBRATION_DPS_LIMIT) ? 1U : 0U;
    if (state->vibration_excessive != 0U)
    {
      consecutive_elevated++;
      if (consecutive_elevated >= MIN_ELEVATED_WINDOWS && vibration_active == 0U)
      {
        state->vibration_event = 1U;
        vibration_active = 1U;
      }
    }
    else
    {
      consecutive_elevated = 0U;
      vibration_active = 0U;
    }
    window_count = 0U;
    window_index = 0U;
  }
  else
  {
    state->vibration_excessive = vibration_active;
  }
  if (state->vibration_excessive) state->vibration_above_ms += elapsed_ms;

  pitch_deg = COMPLEMENTARY_ALPHA * (pitch_deg + sample->gyro_dps[1] * dt) +
              (1.0f - COMPLEMENTARY_ALPHA) * accel_pitch;
  roll_deg = COMPLEMENTARY_ALPHA * (roll_deg + sample->gyro_dps[0] * dt) +
             (1.0f - COMPLEMENTARY_ALPHA) * accel_roll;
  state->tilt_angle_deg = fmaxf(fabsf(pitch_deg), fabsf(roll_deg));
  if (state->tilt_angle_deg > TILT_LIMIT_DEG)
  {
    if (tilt_started == 0U) tilt_started = now_ms;
    state->tilt_outside_ms += elapsed_ms;
  }
  else tilt_started = 0U;
  state->improper_tilt = (tilt_started != 0U && (now_ms - tilt_started) >= TILT_CONFIRM_MS) ? 1U : 0U;
  if (sample->temperature_c < TEMP_MIN_C || sample->temperature_c > TEMP_MAX_C)
  {
    if (temperature_started == 0U) temperature_started = now_ms;
    state->temperature_outside_ms += elapsed_ms;
    state->temperature_excursion = (now_ms - temperature_started >= TEMP_CONFIRM_MS) ? 1U : 0U;
    state->temperature_event = (state->temperature_excursion != 0U &&
                                state->temperature_outside_ms <= elapsed_ms + TEMP_CONFIRM_MS) ? 1U : 0U;
  }
  else
  {
    temperature_started = 0U;
    state->temperature_excursion = 0U;
    state->temperature_event = 0U;
    state->temperature_outside_ms = 0U;
  }
}
