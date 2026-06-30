idf_component_register(
    INCLUDE_DIRS "include"
    SRCS "src/MyLD2410.cpp"
    REQUIRES freertos esp_common driver esp_timer
)
