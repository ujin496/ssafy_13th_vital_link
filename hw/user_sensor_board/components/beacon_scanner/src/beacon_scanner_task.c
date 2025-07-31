#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "wifi_connect.h"

// NimBLE 헤더
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"

static const char *TAG = "BEACON_SCANNER";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static int strongest_rssi = -999;
static uint16_t closest_major = 0;
static uint16_t closest_minor = 0;

// 필터링할 UUID
static const uint8_t TARGET_UUID[16] = {
    0x11, 0x22, 0x33, 0x44,
    0x55, 0x66, 0x77, 0x88,
    0x99, 0x00, 0xaa, 0xbb,
    0xcc, 0xdd, 0xee, 0xff
};

// MQTT로 위치 정보 전송
static void mqtt_send(uint16_t major, uint16_t minor, int rssi) {
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"id\":\"TAG1234\",\"major\":%d,\"minor\":%d,\"rssi\":%d}",
             major, minor, rssi);
    esp_mqtt_client_publish(mqtt_client, "/location/update", payload, 0, 1, 0);
    ESP_LOGI(TAG, "Published: %s", payload);
}

static void ble_app_on_sync(void) {
    ESP_LOGI("BLE", "BLE Host synced");
}

static void nimble_host_task(void *param) {
    nimble_port_run(); // NimBLE BLE host 실행
    nimble_port_freertos_deinit();
}

void ble_init(void) {
    // 💡 NimBLE 초기화 (꼭 필요함!)
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    // 💡 NimBLE을 FreeRTOS 태스크로 실행
    nimble_port_freertos_init(nimble_host_task);
}

// BLE 광고 수신 콜백
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    if (event->type != BLE_GAP_EVENT_DISC) return 0;

    const uint8_t *data = event->disc.data;
    uint8_t len = event->disc.length_data;
    int rssi = event->disc.rssi;

    if (len < 30) return 0;

    // iBeacon: Apple ID 0x004C, Type 0x02, Length 0x15
    if (data[0] == 0x4C && data[1] == 0x00 &&
        data[2] == 0x02 && data[3] == 0x15) {

        const uint8_t *uuid = &data[4];

        if (memcmp(uuid, TARGET_UUID, 16) == 0) {
            uint16_t major = (data[20] << 8) | data[21];
            uint16_t minor = (data[22] << 8) | data[23];

            if (rssi > strongest_rssi) {
                strongest_rssi = rssi;
                closest_major = major;
                closest_minor = minor;
            }
        }
    }

    return 0;
}

// BLE 스캔 Task
void ble_scan_task(void *param) {
    mqtt_client = (esp_mqtt_client_handle_t)param;

    struct ble_gap_disc_params scan_params = {
        .itvl = 0x50,
        .window = 0x30,
        .filter_policy = 0,
        .passive = 0,
        .limited = 0
    };

    while (1) {
        strongest_rssi = -999;

        ESP_ERROR_CHECK(ble_gap_disc(0, BLE_HS_FOREVER, &scan_params, ble_gap_event, NULL));
        vTaskDelay(pdMS_TO_TICKS(3000));  // 스캔 시간 보장

        ESP_LOGI(TAG, "Scan complete. Publishing result...");
        mqtt_send(closest_major, closest_minor, strongest_rssi);

        vTaskDelay(pdMS_TO_TICKS(6000));  // 6초 간격
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "MQTT connected");
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "MQTT disconnected");
    }
}

void ibeacon_tag_start(void) {
    nvs_flash_init();
    ble_init();
    wifi_init_sta();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://172.26.7.105:1883",
        .credentials.username = "a107",
        .credentials.authentication.password = "123456789",
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    xTaskCreate(ble_scan_task, "ble_scan_task", 4096, NULL, 5, NULL);
}
