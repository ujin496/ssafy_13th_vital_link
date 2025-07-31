idf_component_register(
    SRCS "src/beacon_scanner_task.c"
         "src/wifi_connect.c"
         "src/esp_ibeacon_api.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bt nvs_flash
    REQUIRES esp_wifi esp_event esp_netif mqtt
)