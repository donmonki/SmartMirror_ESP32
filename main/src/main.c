
#include "main.h"
#include "wifi_handling.h"
#include "LED_control.h"
#include "ld2410_sensor.h"

uint8_t status_byte = 0;

void GPIO_Config()
{
    // Configure output for RPI
    gpio_reset_pin(TRIGGER_GPIO);
    gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_INPUT_OUTPUT);
}

void app_main(void)
{
    // 1. Initialize Storage and GPIO

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_LED_params();
    GPIO_Config();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
  
    wifi_config_t wifi_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&hybrid_sniffer_cb));

    ESP_LOGI(WIFI_TAG, "Mirror System Initialized. Waiting for Wi-Fi...");
    xTaskCreate(timeout_monitor_task, "timeout_task", TASK_STACK_SIZE_SMALL, NULL, 5, NULL);

    xTaskCreate(ld2410_sensor_task, "ld2410_task", TASK_STACK_SIZE_MEDIUM, NULL, 5, NULL);

    bool led_is_on = false;
    int on_debounce_count = 0;
    int off_debounce_count = 0;
    const int debounce_limit = 5; // in milisec
    const uint8_t light_on_threshold = 48;
    const uint8_t light_off_threshold = 55;
    ESP_LOGI("MAIN", "Entering mirror control loop");

    while (1)
    {
        ld2410_data_t sensor_data = ld2410_get_data();
        bool light_should_turn_on = sensor_data.light_level <= light_on_threshold;
        bool light_should_turn_off = sensor_data.light_level > light_off_threshold;
        bool should_be_on = (sensor_data.output_level == 1) && light_should_turn_on;
        bool should_turn_off = (sensor_data.output_level == 0) || light_should_turn_off;

        if (should_be_on) {
            on_debounce_count++;
            off_debounce_count = 0;
        } else if (should_turn_off) {
            off_debounce_count++;
            on_debounce_count = 0;
        }

        bool debounced_on = (on_debounce_count >= debounce_limit) || (led_is_on && off_debounce_count < debounce_limit);
        ESP_LOGI("MAIN", "STATUS: output_level=%d, light_level=%d, should_be_on=%d, should_turn_off=%d, debounced_on=%d",
            sensor_data.output_level,
            sensor_data.light_level,
            should_be_on,
            should_turn_off,
            debounced_on);

        if (debounced_on && !led_is_on) {
            ESP_LOGI("MAIN", "LED trigger ON: output_level=%d, light_level=%d",
                     sensor_data.output_level,
                     sensor_data.light_level);
            led_strip_ON_sequence_soft();
            led_is_on = true;
        } else if (!debounced_on && led_is_on) {
            ESP_LOGI("MAIN", "LED trigger OFF: output_level=%d, light_level=%d",
                     sensor_data.output_level,
                     sensor_data.light_level);
            led_strip_OFF_sequence_soft();
            led_is_on = false;
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }

        
    
}

//  esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     GPIO_Config();

//     // 2. Initialize Wi-Fi
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
//     esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

//     wifi_config_t wifi_config = {
//         .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}};
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     // 3. CRITICAL: Disable Power Save so sniffer doesn't miss packets
//     esp_wifi_set_ps(WIFI_PS_NONE);

//     //4. Start Sniffer
//     ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
//     ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&hybrid_sniffer_cb));

//     ESP_LOGI(WIFI_TAG, "Mirror System Initialized. Waiting for Wi-Fi...");
//     xTaskCreate(timeout_monitor_task, "timeout_task", TASK_STACK_SIZE_SMALL, NULL, 5, NULL);
