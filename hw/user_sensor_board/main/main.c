// 예시 메인 코드
// app_main() -> FreeRTOS 진입점

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "gyro_sensor_task.h"
// #include "heart_sensor_task.h"
// #include "temp_sensor_task.h"
#include "beacon_scanner_task.h"

void app_main(void) {
    ibeacon_tag_start();
    // gyro_sensor_start();
}
