idf_component_register(
    SRCS "control_task.c" "udp_task.c" "sensor_task.c" "process_task.c" "my_esp32s3_app.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_include"
    REQUIRES
        qma6100p
        wifi_sta
        esp_timer
        driver
        lwip
)