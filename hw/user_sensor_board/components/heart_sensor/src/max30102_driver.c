#include "max30102_driver.h"
#include "driver/i2c.h"

#define MAX30102_ADDR         0x57
#define REG_MODE_CONFIG       0x09
#define REG_FIFO_WR_PTR       0x04
#define REG_FIFO_RD_PTR       0x06
#define REG_FIFO_DATA         0x07
#define REG_FIFO_CONFIG       0x08
#define REG_SPO2_CONFIG       0x0A
#define REG_LED1_PA           0x0C
#define REG_LED2_PA           0x0D
#define REG_INT_ENABLE_1      0x02

static esp_err_t write_register(i2c_port_t port, uint8_t reg, uint8_t val) {
    uint8_t data[2] = {reg, val};
    return i2c_master_write_to_device(port, MAX30102_ADDR, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_register(i2c_port_t port, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(port, MAX30102_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t max30102_init(i2c_port_t port) {
    // Reset
    write_register(port, REG_MODE_CONFIG, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));

    // FIFO & SpO2 Config
    write_register(port, REG_FIFO_CONFIG, 0x0F);    // Sample avg = 4, FIFO rollover = disabled
    write_register(port, REG_MODE_CONFIG, 0x03);    // SpO2 mode
    write_register(port, REG_SPO2_CONFIG, 0x27);    // 100Hz, 411us pulse width
    write_register(port, REG_LED1_PA, 0x24);        // IR LED power
    write_register(port, REG_LED2_PA, 0x24);        // RED LED power

    return ESP_OK;
}

esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir) {
    uint8_t data[6];
    esp_err_t err = read_register(I2C_NUM_0, REG_FIFO_DATA, data, 6);
    if (err != ESP_OK) return err;

    *red = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    *red &= 0x3FFFF;

    *ir = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    *ir &= 0x3FFFF;

    return ESP_OK;
}
