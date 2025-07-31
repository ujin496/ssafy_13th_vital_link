#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050_driver.h"
#include "gyro_sensor_task.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_SCL_IO     22
#define I2C_SDA_IO     21
#define I2C_FREQ_HZ    400000

static const char *TAG = "GYRO";

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

        vTaskDelay(pdMS_TO_TICKS(20));  // 약 50Hz 주기
    }
}

void gyro_sensor_start(void) {
    i2c_master_init();

    if (mpu6050_init(I2C_MASTER_NUM) == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 initialized successfully");
    } else {
        ESP_LOGE(TAG, "MPU6050 initialization failed");
    }

    xTaskCreate(gyro_task, "gyro_task", 2048, NULL, 5, NULL);
}