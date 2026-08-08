#include "main.h"
#include "app_config.h"
#include "mpu6050.h"
#include "uart_logger.h"
#include "health_monitor.h"
#include "health_score.h"

static button_t *user_button;
static volatile uint8_t button_event_pending;
static volatile uint8_t trip_active;
static volatile uint8_t sample_pending;
static volatile uint8_t sample_divider;

/* Called from the 1 kHz SysTick ISR. It only schedules foreground work. */
void app_systick_hook(void)
{
  if (trip_active == 0U)
  {
    sample_divider = 0U;
    sample_pending = 0U;
    return;
  }
  sample_divider++;
  if (sample_divider >= (1000U / APP_SAMPLE_RATE_HZ))
  {
    sample_divider = 0U;
    if (sample_pending < 255U)
    {
      sample_pending++;
    }
  }
}

static void button_callback(button_t *button, void *arg)
{
  (void)arg;
  (void)button;
  button_event_pending = 1U;
}

int main(void)
{
  mpu6050_sample_t sample = {0};
  uint8_t sensor_ok;
  uint32_t next_sample;
  uint32_t next_telemetry;
  health_state_t health = {0};
  health_state_t previous_health = {0};
  health_score_t score;

  if (mx_system_init() != SYSTEM_OK)
  {
    return -1;
  }

  uart_logger_init();
  sensor_ok = (mpu6050_init() == MPU6050_OK) ? 1U : 0U;
  uart_logger_message(sensor_ok ? "MPU6050 ready on I2C1 PB6/PB7" : "MPU6050 not detected");

  user_button = mx_button_getobject();
  if (button_init(user_button, MX_BUTTON) != BUTTON_OK ||
      button_register_callback(user_button, button_callback, BUTTON_EVENT_PRESSED, NULL) != BUTTON_OK ||
      button_enableit(user_button) != BUTTON_OK)
  {
    uart_logger_message("!!BUTTON!! initialization failed");
    return -1;
  }

  next_sample = HAL_GetTick();
  next_telemetry = next_sample;
  health_score_init(&score, next_sample);
  health_monitor_init(&health, next_sample);
  trip_active = 0U;
  button_event_pending = 0U;
  uart_logger_message("Press onboard button to start trip");

  while (1)
  {
    uint32_t now = HAL_GetTick();

    if (button_event_pending != 0U)
    {
      button_event_pending = 0U;
      trip_active = (trip_active == 0U) ? 1U : 0U;

      if (trip_active != 0U)
      {
        health_score_reset(&score, now);
        health_monitor_init(&health, now);
        previous_health = (health_state_t){0};
        next_sample = now;
        next_telemetry = now;
        uart_logger_message("TRIP STARTED");
      }
      else
      {
        /* Stop all sample/event/telemetry output. Only the transition line is sent. */
        health_score_reset(&score, now);
        uart_logger_message("TRIP ENDED");
      }
    }

    if (trip_active == 0U)
    {
      continue;
    }

      if (sample_pending != 0U)
    {
      sample_pending--;
      if (sensor_ok != 0U && mpu6050_read_sample(&sample) != MPU6050_OK)
      {
        sensor_ok = 0U;
        uart_logger_message("!!SENSOR!! MPU6050 read fault");
      }
      else if (sensor_ok != 0U)
      {
        health_monitor_process(&health, &sample, now, 1000U / APP_SAMPLE_RATE_HZ);
        health_score_update(&score, &health, now, 1000U / APP_SAMPLE_RATE_HZ);
        uart_logger_events(now, &health, &previous_health, &score);
        previous_health = health;
      }
    }

    if ((int32_t)(now - next_telemetry) >= 0)
    {
      next_telemetry += (1000U / APP_TELEMETRY_RATE_HZ);
      uart_logger_sample(now, &sample, &health, &score, sensor_ok);
    }
  }
}
