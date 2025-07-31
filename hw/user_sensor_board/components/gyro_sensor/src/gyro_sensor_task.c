#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "gyro_sensor_task.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_SCL_IO     22
#define I2C_SDA_IO     21
#define I2C_FREQ_HZ    400000

static const char *TAG = "GYRO";

esp_err_t mpu6050_init(i2c_port_t i2c_num) {
    uint8_t data;

    // Wake up device
    data = 0x00;
    ESP_ERROR_CHECK(i2c_master_write_to_device(i2c_num, MPU6050_ADDR, &data, 1, pdMS_TO_TICKS(100)));

    // Set PWR_MGMT_1 to 0 (wake up)
    return i2c_master_write_to_device(i2c_num, MPU6050_ADDR, (uint8_t[]){MPU6050_PWR_MGMT_1, 0}, 2, pdMS_TO_TICKS(100));
}

esp_err_t mpu6050_read_data(i2c_port_t i2c_num, mpu6050_data_t* data) {
    uint8_t buffer[14];
    esp_err_t err;

    err = i2c_master_write_read_device(i2c_num, MPU6050_ADDR,
                                       (uint8_t[]){MPU6050_ACCEL_XOUT_H}, 1,
                                       buffer, sizeof(buffer),
                                       pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;

    data->ax = (buffer[0] << 8) | buffer[1];
    data->ay = (buffer[2] << 8) | buffer[3];
    data->az = (buffer[4] << 8) | buffer[5];
    data->gx = (buffer[8] << 8) | buffer[9];
    data->gy = (buffer[10] << 8) | buffer[11];
    data->gz = (buffer[12] << 8) | buffer[13];

    return ESP_OK;
}

void i2c_master_init(void) {
    static bool initialized = false;
    if (initialized) return;  // 이미 초기화 됐으면 스킵
    initialized = true;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void gyro_task(void *pvParameters) {
    mpu6050_data_t sensor_data;

    while (1) {
        if (mpu6050_read_data(I2C_MASTER_NUM, &sensor_data) == ESP_OK) {
            printf("Accel: X=%d, Y=%d, Z=%d | Gyro: X=%d, Y=%d, Z=%d\n",
                   sensor_data.ax, sensor_data.ay, sensor_data.az,
                   sensor_data.gx, sensor_data.gy, sensor_data.gz);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void gyro_sensor_start(void) {
    i2c_master_init();
    mpu6050_init(I2C_MASTER_NUM);
    xTaskCreate(gyro_task, "gyro_task", 2048, NULL, 5, NULL);
}
