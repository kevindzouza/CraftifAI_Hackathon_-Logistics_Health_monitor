#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "mpu6050.h"
#include <stdint.h>

typedef struct
{
  float acceleration_magnitude_g;
  float tilt_angle_deg;
  float gyro_rms_dps;
  float accel_vibration_rms_g;
  uint8_t shock_level;
  uint8_t vibration_excessive;
  uint8_t vibration_event;
  uint8_t shock_event;
  uint8_t improper_tilt;
  uint8_t temperature_excursion;
  uint32_t vibration_above_ms;
  uint32_t tilt_outside_ms;
  uint32_t temperature_outside_ms;
  uint8_t temperature_event;
} health_state_t;

void health_monitor_init(health_state_t *state, uint32_t now_ms);
void health_monitor_process(health_state_t *state, const mpu6050_sample_t *sample,
                            uint32_t now_ms, uint32_t elapsed_ms);

#endif
