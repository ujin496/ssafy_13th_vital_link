#include "mlx90614_driver.h"
#include "driver/i2c.h"

#define MLX90614_ADDR  0x5A
#define REG_OBJECT_TEMP 0x07

esp_err_t mlx90614_read_temp(float *object_temp) {
    uint8_t data[3];
    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, MLX90614_ADDR,
                                                 (uint8_t[]){REG_OBJECT_TEMP}, 1, data, 3, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    uint16_t raw = data[0] | (data[1] << 8);
    *object_temp = (float)raw * 0.02 - 273.15;
    return ESP_OK;
}
