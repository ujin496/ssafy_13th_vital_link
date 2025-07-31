#include "mpu6050_driver.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define MPU6050_ADDR           0x68
#define MPU6050_PWR_MGMT_1     0x6B
#define MPU6050_ACCEL_XOUT_H   0x3B
#define I2C_TIMEOUT_MS         100

static const char* TAG = "MPU6050";

esp_err_t mpu6050_init(i2c_port_t port) {
    uint8_t data[2];

    // Wake up sensor (clear sleep bit)
    data[0] = MPU6050_PWR_MGMT_1;
    data[1] = 0x00;
    esp_err_t err = i2c_master_write_to_device(port, MPU6050_ADDR, data, 2, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init MPU6050: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t mpu6050_read_data(i2c_port_t port, mpu6050_data_t *out_data) {
    uint8_t reg = MPU6050_ACCEL_XOUT_H;
    uint8_t buffer[14];

    esp_err_t err = i2c_master_write_read_device(port, MPU6050_ADDR,
                                                 &reg, 1,
                                                 buffer, sizeof(buffer),
                                                 pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MPU6050 data: %s", esp_err_to_name(err));
        return err;
    }

    out_data->ax = (int16_t)(buffer[0] << 8 | buffer[1]);
    out_data->ay = (int16_t)(buffer[2] << 8 | buffer[3]);
    out_data->az = (int16_t)(buffer[4] << 8 | buffer[5]);
    out_data->gx = (int16_t)(buffer[8] << 8 | buffer[9]);
    out_data->gy = (int16_t)(buffer[10] << 8 | buffer[11]);
    out_data->gz = (int16_t)(buffer[12] << 8 | buffer[13]);

    return ESP_OK;
}
