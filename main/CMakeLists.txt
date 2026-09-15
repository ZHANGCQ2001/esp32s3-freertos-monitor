idf_component_register(
    SRCS "my_esp32s3_app.c"
    INCLUDE_DIRS "."
    REQUIRES
        qma6100p
        esp_timer
        driver
)