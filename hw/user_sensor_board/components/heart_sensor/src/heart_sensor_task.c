// 실제 심박 측정 X, 데이터 받아오는 데모 코드

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "max30102_driver.h"
#include "heart_sensor_task.h"

#define I2C_MASTER_NUM I2C_NUM_0

static const char *TAG = "HEART";

static void i2c_master_init(void) {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

static void heart_task(void *pvParameters) {
    uint32_t red, ir;
    while (1) {
        if (max30102_read_fifo(&red, &ir) == ESP_OK) {
            ESP_LOGI(TAG, "RED: %lu | IR: %lu", red, ir);
        } else {
            ESP_LOGW(TAG, "Read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void heart_sensor_start(void) {
    i2c_master_init();
    max30102_init(I2C_MASTER_NUM);
    xTaskCreate(heart_task, "heart_task", 2048, NULL, 5, NULL);
}
