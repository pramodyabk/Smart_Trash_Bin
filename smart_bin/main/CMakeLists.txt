idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES mqtt esp32-camera esp-tflite-micro nvs_flash esp_wifi)