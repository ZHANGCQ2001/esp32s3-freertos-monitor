idf_component_register(
    SRCS "wifi_sta.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_include"
    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
)