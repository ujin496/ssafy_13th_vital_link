// FreeRTOS 구조만 살린 데모 코드
// MLX90614 라이브러리 이용하여 실제 센서에서 데이터 받아와야 함

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "mlx90614_driver.h"
#include "temp_sensor_task.h"

#define I2C_MASTER_NUM I2C_NUM_0
static const char *TAG = "TEMP";

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

static void temp_task(void *pvParameters) {
    float temp;
    while (1) {
        if (mlx90614_read_temp(&temp) == ESP_OK) {
            ESP_LOGI(TAG, "Object Temp: %.2f°C", temp);
        } else {
            ESP_LOGW(TAG, "Temp read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void temp_sensor_start(void) {
    i2c_master_init();
    xTaskCreate(temp_task, "temp_task", 2048, NULL, 5, NULL);
}
