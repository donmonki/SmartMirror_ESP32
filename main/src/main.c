
#include "main.h"
#include "wifi_handling.h"

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

    GPIO_Config();

    // 2. Initialize Wi-Fi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 3. CRITICAL: Disable Power Save so sniffer doesn't miss packets
    esp_wifi_set_ps(WIFI_PS_NONE);

    // 4. Start Sniffer
    // ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    // ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&hybrid_sniffer_cb));

    ESP_LOGI(WIFI_TAG, "Mirror System Initialized. Waiting for Wi-Fi...");
    xTaskCreate(timeout_monitor_task, "timeout_task", TASK_STACK_SIZE_SMALL, NULL, 5, NULL);
}