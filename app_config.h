#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Board-grounded Arduino I2C mapping: I2C1 on PB6=SCL and PB7=SDA.
 * The STM32Cube C5 eeprom_dma example confirms these pins for NUCLEO-C5A3ZG.
 * The example's generated timing is 100 kHz; keep this conservative until the
 * MPU6050 wiring and pull-ups are validated. */
#define APP_I2C_INSTANCE              HAL_I2C1
#define APP_I2C_TIMING                0x70D25153U /* 100 kHz, 144 MHz I2C kernel */
#define APP_I2C_SCL_PIN               HAL_GPIO_PIN_6
#define APP_I2C_SDA_PIN               HAL_GPIO_PIN_7
#define APP_I2C_GPIO_PORT             HAL_GPIOB
#define APP_I2C_GPIO_AF               HAL_GPIO_AF_4

/* MPU6050 uses the 7-bit address 0x68 when AD0 is low, 0x69 when AD0 is high.
 * HAL I2C APIs expect the address shifted left by one bit. */
#define APP_MPU6050_ADDRESS_PRIMARY   0x68U
#define APP_MPU6050_ADDRESS_SECONDARY 0x69U
#define APP_MPU6050_ACCEL_RANGE_G     8U
#define APP_MPU6050_GYRO_RANGE_DPS    500U
#define APP_SAMPLE_RATE_HZ             200U /* brief shock pulses need >100 Hz */
#define APP_TELEMETRY_RATE_HZ         2U

/* Tilt event policy: tilt must exceed 40 degrees continuously for 2 seconds. */
#define APP_TILT_LIMIT_DEG            40.0f
#define APP_TILT_CONFIRM_MS           2000U
#define APP_TILT_SCORE_PENALTY        10
#define APP_TEMP_MIN_C                0.0f
#define APP_TEMP_MAX_C                35.0f
#define APP_TEMP_CONFIRM_MS           3000U
#define APP_TEMP_SCORE_PENALTY        10
#define APP_MPU6050_DLPF_CONFIG       2U /* 98 Hz accel / 98 Hz gyro bandwidth */
#define APP_MPU6050_SMPLRT_DIV        4U /* 1 kHz/(1+4) = 200 Hz with DLPF on */

#endif
