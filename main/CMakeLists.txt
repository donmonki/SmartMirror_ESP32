idf_component_register(
    SRCS 
        src/main.c
        src/wifi_handling.c
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
    PRIV_REQUIRES       # optional, list the private requirements
)