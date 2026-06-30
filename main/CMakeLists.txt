idf_component_register(
    SRCS 
        src/main.c
        src/wifi_handling.c
        src/LED_control.c
        src/ld2410_sensor.cpp
    INCLUDE_DIRS 
        include         # Public headers accessible to other components
    REQUIRES 
        nvs_flash 
        freertos 
        esp_timer 
        driver 
        log
        esp_wifi
        esp_netif
        esp_event
        lwip
        MyLD2410
    PRIV_REQUIRES       # optional, list the private requirements
)