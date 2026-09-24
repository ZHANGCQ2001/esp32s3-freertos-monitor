idf_component_register(
    SRCS "udp_task.c" "sensor_task.c" "process_task.c" "my_esp32s3_app.c"
    INCLUDE_DIRS "include"
    REQUIRES
        qma6100p
        wifi_sta
        esp_timer
        driver
)