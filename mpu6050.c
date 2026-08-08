#include "mpu6050.h"
#include "app_config.h"
#include "mx_i2c1.h"
#include <stddef.h>

#define MPU_REG_SMPLRT_DIV   0x19U
#define MPU_REG_CONFIG       0x1AU
#define MPU_REG_GYRO_CONFIG  0x1BU
#define MPU_REG_ACCEL_CONFIG 0x1CU
#define MPU_REG_ACCEL_XOUT_H 0x3BU
#define MPU_REG_PWR_MGMT_1   0x6BU
#define MPU_REG_WHO_AM_I     0x75U
#define MPU_WHO_AM_I_VALUE   0x68U
#define MPU_I2C_TIMEOUT_MS   5U

static hal_i2c_handle_t *i2c_handle;
static uint8_t device_address;
static float accel_scale;
static float gyro_scale;

static hal_status_t reg_write(uint8_t reg, uint8_t value)
{
  return HAL_I2C_MASTER_MemWrite(i2c_handle, (uint32_t)(device_address << 1U), reg,
                                 HAL_I2C_MEM_ADDR_8BIT, &value, 1U, MPU_I2C_TIMEOUT_MS);
}

static hal_status_t reg_read(uint8_t reg, uint8_t *data, uint32_t length)
{
  return HAL_I2C_MASTER_MemRead(i2c_handle, (uint32_t)(device_address << 1U), reg,
                                HAL_I2C_MEM_ADDR_8BIT, data, length, MPU_I2C_TIMEOUT_MS);
}

static mpu6050_status_t bus_init(void)
{
  /* mx_system_init() initializes I2C1, its PB6/PB7 alternate-function pins,
   * DMA channels, and I2C interrupts before application code starts. */
  i2c_handle = mx_i2c1_i2c_gethandle();
  return (i2c_handle != NULL) ? MPU6050_OK : MPU6050_ERROR;
}

mpu6050_status_t mpu6050_init(void)
{
  uint8_t who = 0U;

  if (bus_init() != MPU6050_OK)
  {
    return MPU6050_ERROR;
  }

  device_address = APP_MPU6050_ADDRESS_PRIMARY;
  if (reg_read(MPU_REG_WHO_AM_I, &who, 1U) != HAL_OK || who != MPU_WHO_AM_I_VALUE)
  {
    device_address = APP_MPU6050_ADDRESS_SECONDARY;
    if (reg_read(MPU_REG_WHO_AM_I, &who, 1U) != HAL_OK || who != MPU_WHO_AM_I_VALUE)
    {
      return MPU6050_ERROR;
    }
  }

  /* DLPF_CFG=2 gives a 98 Hz bandwidth; with the 1 kHz internal rate,
   * SMPLRT_DIV=4 produces 200 samples/s without smearing short shocks. */
  if (reg_write(MPU_REG_PWR_MGMT_1, 0x01U) != HAL_OK ||
      reg_write(MPU_REG_CONFIG, APP_MPU6050_DLPF_CONFIG) != HAL_OK ||
      reg_write(MPU_REG_SMPLRT_DIV, APP_MPU6050_SMPLRT_DIV) != HAL_OK)
  {
    return MPU6050_ERROR;
  }

  accel_scale = (APP_MPU6050_ACCEL_RANGE_G == 2U) ? 16384.0f :
                (APP_MPU6050_ACCEL_RANGE_G == 4U) ? 8192.0f :
                (APP_MPU6050_ACCEL_RANGE_G == 8U) ? 4096.0f : 2048.0f;
  gyro_scale = (APP_MPU6050_GYRO_RANGE_DPS == 250U) ? 131.0f :
               (APP_MPU6050_GYRO_RANGE_DPS == 500U) ? 65.5f :
               (APP_MPU6050_GYRO_RANGE_DPS == 1000U) ? 32.8f : 16.4f;

  if (reg_write(MPU_REG_GYRO_CONFIG, (uint8_t)(((APP_MPU6050_GYRO_RANGE_DPS == 250U) ? 0U :
                                                (APP_MPU6050_GYRO_RANGE_DPS == 500U) ? 1U :
                                                (APP_MPU6050_GYRO_RANGE_DPS == 1000U) ? 2U : 3U) << 3U)) != HAL_OK ||
      reg_write(MPU_REG_ACCEL_CONFIG, (uint8_t)(((APP_MPU6050_ACCEL_RANGE_G == 2U) ? 0U :
                                                 (APP_MPU6050_ACCEL_RANGE_G == 4U) ? 1U :
                                                 (APP_MPU6050_ACCEL_RANGE_G == 8U) ? 2U : 3U) << 3U)) != HAL_OK)
  {
    return MPU6050_ERROR;
  }

  return MPU6050_OK;
}

mpu6050_status_t mpu6050_read_sample(mpu6050_sample_t *sample)
{
  uint8_t raw[14];
  int16_t value[7];

  if (sample == NULL || reg_read(MPU_REG_ACCEL_XOUT_H, raw, sizeof(raw)) != HAL_OK)
  {
    return MPU6050_ERROR;
  }

  for (uint32_t i = 0U; i < 7U; ++i)
  {
    value[i] = (int16_t)(((uint16_t)raw[2U * i] << 8U) | raw[(2U * i) + 1U]);
  }

  sample->accel_g[0] = (float)value[0] / accel_scale;
  sample->accel_g[1] = (float)value[1] / accel_scale;
  sample->accel_g[2] = (float)value[2] / accel_scale;
  sample->temperature_c = ((float)value[3] / 340.0f) + 36.53f;
  sample->gyro_dps[0] = (float)value[4] / gyro_scale;
  sample->gyro_dps[1] = (float)value[5] / gyro_scale;
  sample->gyro_dps[2] = (float)value[6] / gyro_scale;
  sample->who_am_i = MPU_WHO_AM_I_VALUE;
  return MPU6050_OK;
}

uint8_t mpu6050_get_address(void)
{
  return device_address;
}
