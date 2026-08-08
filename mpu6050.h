#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "stm32_hal.h"

typedef struct
{
  float accel_g[3];
  float gyro_dps[3];
  float temperature_c;
  uint8_t who_am_i;
} mpu6050_sample_t;

typedef enum { MPU6050_OK = 0, MPU6050_ERROR = 1 } mpu6050_status_t;

mpu6050_status_t mpu6050_init(void);
mpu6050_status_t mpu6050_read_sample(mpu6050_sample_t *sample);
uint8_t mpu6050_get_address(void);

#endif
