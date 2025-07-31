// FreeRTOS 기반 Task 구조의 ibeacon_tag.c 파일

#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_ibeacon_api.h"
#include "mqtt_client.h"
#include "wifi_connect.h"
#include "beacon_scanner_task.h"

static const char *TAG = "IBEACON_TAG";     // 로그 출력용 태그

// MQTT 클라이언트 핸들 선언 및 변수 초기화
static esp_mqtt_client_handle_t mqtt_client = NULL;
static int strongest_rssi = -999;
static uint16_t closest_major = 0;
static uint16_t closest_minor = 0;

// 가장 가까운 iBeacon 정보를 MQTT로 전송하는 함수
static void mqtt_send(uint16_t major, uint16_t minor, int rssi) {
    char payload[128];
    // JSON 형식으로 major, minor, rssi 정보 포맷팅
    snprintf(payload, sizeof(payload),
             "{\"id\":\"TAG1234\",\"major\":%d,\"minor\":%d,\"rssi\":%d}",
             major, minor, rssi);
    // MQTT publish 수행
    esp_mqtt_client_publish(mqtt_client, "/location/update", payload, 0, 1, 0);
    // 디버깅 or 상태 확인용 메시지 콘솔 출력
    ESP_LOGI(TAG, "Published: %s", payload);    
}

// BLE 스캔 콜백 함수 - iBeacon 패킷 필터링, RSSI 신호 비교하여 가장 가까운 Anchor로 현재 구역 판별
// [의문점] 콜백 함수가 뭐지?
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            // iBeacon 패킷인지 확인
            if (esp_ble_is_ibeacon_packet(param->scan_rst.ble_adv, param->scan_rst.adv_data_len)) {
                esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t *)param->scan_rst.ble_adv;
                int rssi = param->scan_rst.rssi;
                // 빅 엔디안에서 리틀 엔디안으로 변환
                // [의문점] 엔디안이 뭐지?
                uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
                uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
                
                // 현재까지 가장 강한 신호보다 강하면 저장
                if (rssi > strongest_rssi) {
                    strongest_rssi = rssi;
                    closest_major = major;
                    closest_minor = minor;
                }
            }
        }
    }
}

// BLE 스캔을 주기적으로 수행하는 Task 함수(FreeRTOS Task)
static void ble_scan_task(void* arg) {
    // BLE GAP 콜백 함수 등록
    esp_ble_gap_register_callback(esp_gap_cb);

    esp_ble_scan_params_t scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = 0x50,
        .scan_window            = 0x30,
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
    };

    esp_ble_gap_set_scan_params(&scan_params);

    // 주변 Beacon 신호 스캔
    // [의문점] 스캔 주기를 짧게 하면 어떤 일이?
    // [의문점] vTaskDelay의 사용 이유(FreeRTOS 라이브러리)
    while (1) {
        strongest_rssi = -999;

        esp_ble_gap_start_scanning(1);  // 1초 스캔 - 스캔하기엔 시간이 너무 짧다. 스캔에 은근 시간이 오래 걸림.
        // vTaskDelay(): 현재 실행 중인 Task 일시 중단(Delay)
        // pdMS_TO_TICKS: 1.5초를 FreeRTOS Tick 단위로 변환
        vTaskDelay(pdMS_TO_TICKS(1500));  // 스캔 여유 시간 대기

        mqtt_send(closest_major, closest_minor, strongest_rssi);    // MQTT 전송
        vTaskDelay(pdMS_TO_TICKS(1000));  // 다음 스캔까지 대기
    }
}

// MQTT 이벤트 핸들러 - 연결/끊김 상태 확인
// [의문점] 이벤트 핸들러가 뭐지? 왜 사용하는 걸까
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        default:
            break;
    }
}

// BLE 초기화 함수(BLE 전용 모드로 설정 및 초기화)
// [의문점] 이 설정은 menuconfig 수동 변경 설정과는 다른가?
static void ble_init() {
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
}

// app_main에서 실행되는 iBeacon Tag 함수 - 전체 시스템 초기화 및 시작
void ibeacon_tag_start(void) {
    nvs_flash_init();   // NVS 플래시 초기화(Wi-Fi 및 BLE 설정 저장소)
    wifi_init_sta();    // Wi-Fi 연결(station 모드)

    // MQTT 클라이언트 설정 및 시작
    // [의문점] MQTT 클라이언트가 ESP32인가? 이벤트 핸들러는 뭐고 왜 등록하는 거지?
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://172.26.7.105:1883",   // EC2 서버의 IPv4 주소
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);  // MQTT 클라이언트 초기화
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);    // 이벤트 등록
    esp_mqtt_client_start(mqtt_client);

    // BLE 스택 초기화 및 BLE 스캔 Task 생성(등록)
    ble_init();
    xTaskCreate(ble_scan_task, "ble_scan_task", 4096, NULL, 5, NULL);   // FreeRTOS Task 등록
}
