#include "uart_logger.h"
#include "mx_basic_stdio_app.h"
#include <stdio.h>

void uart_logger_init(void)
{
  (void)mx_basic_stdio_init();
  printf("\r\nLogistics package health monitor\r\n");
  printf("UART: USART2 DBGIN_VCP_TX PA2, 115200 8N1\r\n");
}

void uart_logger_message(const char *message)
{
  printf("%s\r\n", message);
}

static void print_fixed(float value, uint8_t decimals)
{
  int32_t divisor = (decimals == 1U) ? 10 : 100;
  int32_t scaled = (int32_t)(value * (float)divisor);
  int32_t whole = scaled / divisor;
  int32_t fraction = scaled % divisor;
  if (scaled < 0 && fraction > 0) fraction = -fraction;
  printf(decimals == 1U ? "%ld.%01ld" : "%ld.%02ld", (long)whole,
         (long)((fraction < 0) ? -fraction : fraction));
}

void uart_logger_sample(uint32_t timestamp_ms, const mpu6050_sample_t *sample,
                        const health_state_t *health, const health_score_t *score,
                        uint8_t sensor_ok)
{
  if (sensor_ok == 0U)
  {
    printf("[%lu ms] MPU6050=FAULT\r\n", (unsigned long)timestamp_ms);
    return;
  }
  printf("[%lu ms] ACC(g): x=", (unsigned long)timestamp_ms);
  print_fixed(sample->accel_g[0], 2U); printf(" y="); print_fixed(sample->accel_g[1], 2U);
  printf(" z="); print_fixed(sample->accel_g[2], 2U);
  printf(" | GYRO(dps): x="); print_fixed(sample->gyro_dps[0], 1U);
  printf(" y="); print_fixed(sample->gyro_dps[1], 1U);
  printf(" z="); print_fixed(sample->gyro_dps[2], 1U);
  printf(" | TEMP="); print_fixed(sample->temperature_c, 1U);
  printf("C | MAG="); print_fixed(health->acceleration_magnitude_g, 2U);
  printf("g | VIB="); print_fixed(health->gyro_rms_dps, 1U);
  printf("dps | ACCVIB="); print_fixed(health->accel_vibration_rms_g, 2U);
  printf("g | TILT="); print_fixed(health->tilt_angle_deg, 1U);
  printf("deg | SCORE=%ld | STATUS=%s\r\n", (long)score->live_score,
         (health->shock_level != 0U || health->vibration_excessive != 0U ||
          health->improper_tilt != 0U || health->temperature_excursion != 0U) ? "EVENT" : "OK");
}

void uart_logger_events(uint32_t timestamp_ms, const health_state_t *health,
                        const health_state_t *previous, const health_score_t *score)
{
  if (health->shock_level != 0U && previous->shock_level == 0U)
  {
    printf("!!SHOCK!! level=%u peak=", (unsigned)health->shock_level);
    print_fixed(health->acceleration_magnitude_g, 2U);
    printf("g t=%lu ms SCORE=%ld\r\n", (unsigned long)timestamp_ms, (long)score->live_score);
  }
  if (health->vibration_event != 0U)
  {
    printf("!!VIBRATION!! gyro_rms="); print_fixed(health->gyro_rms_dps, 1U);
    printf(" accel_rms="); print_fixed(health->accel_vibration_rms_g, 2U);
    printf(" dps t=%lu ms\r\n", (unsigned long)timestamp_ms);
  }
  if (health->improper_tilt != 0U && previous->improper_tilt == 0U)
  {
    printf("!!TILT!! angle="); print_fixed(health->tilt_angle_deg, 1U);
    printf(" deg t=%lu ms\r\n", (unsigned long)timestamp_ms);
  }
  if (health->temperature_event != 0U)
    printf("!!TEMP!! excursion duration=%lu ms t=%lu ms\r\n", (unsigned long)health->temperature_outside_ms, (unsigned long)timestamp_ms);
}
