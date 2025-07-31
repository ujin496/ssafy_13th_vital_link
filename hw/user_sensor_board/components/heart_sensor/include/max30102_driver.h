#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t max30102_init(i2c_port_t i2c_num);
esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir);
