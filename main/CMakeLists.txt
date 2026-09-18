idf_component_register(
    SRCS "sensor_task.c" "process_task.c" "my_esp32s3_app.c"
    INCLUDE_DIRS "include"
    REQUIRES
        qma6100p
        esp_timer
        driver
)