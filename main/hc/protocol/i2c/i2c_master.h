#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

void i2c_master_init(void);
uint8_t i2c_send_bytes(uint8_t *data, size_t len, uint8_t slave_addr);
uint8_t i2c_read_bytes(uint8_t *data, size_t len, uint8_t slave_addr);
uint8_t i2c_read_register(uint8_t target_index, uint8_t slave_addr);

uint8_t GetSttGroupRelayPos(uint8_t pos);
void SetPowerOnPos(uint8_t pos);
#ifdef __cplusplus
}
#endif