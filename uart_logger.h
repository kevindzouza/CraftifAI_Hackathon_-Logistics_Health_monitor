#ifndef UART_LOGGER_H
#define UART_LOGGER_H

#include "mpu6050.h"
#include "health_monitor.h"
#include "health_score.h"
#include <stdint.h>

void uart_logger_init(void);
void uart_logger_message(const char *message);
void uart_logger_sample(uint32_t timestamp_ms, const mpu6050_sample_t *sample,
                        const health_state_t *health, const health_score_t *score,
                        uint8_t sensor_ok);
void uart_logger_events(uint32_t timestamp_ms, const health_state_t *health,
                        const health_state_t *previous, const health_score_t *score);

#endif
