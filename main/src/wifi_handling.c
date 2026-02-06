#include "main.h"

const uint8_t target_mac[TARGET_MAC_LEN] = TARGET_MAC_ADDR;
esp_ping_handle_t ping_handle = NULL;
uint32_t last_seen_ms = 0;
TaskHandle_t ping_task_handle = NULL;


// Use a Task Handle to start the task later

void timeout_monitor_task(void *pvParameters)
{
    while (1)
    {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        // ESP_LOGW(WIFI_TAG, "Now time: %d", now);
        // ESP_LOGW(WIFI_TAG, "Last_seen time: %d", last_seen_ms);
        // ESP_LOGW(WIFI_TAG, "GPIO LEVEL: %d", gpio_get_level(TRIGGER_GPIO));

        // If current time minus last seen time is greater than timeout

        if (now - last_seen_ms > TIMEOUT_MS)
        {
            if (gpio_get_level(TRIGGER_GPIO) == 1)
            {
                status_byte &= ~((1<<FLAG_PING_SUCCESS) | (1 << FLAG_SNIFFER_SUCCESS));
                gpio_set_level(TRIGGER_GPIO, 0);
                ESP_LOGE(WIFI_TAG, "Timeout Reached: iPhone not seen for %d seconds. Mirror OFF.", TIMEOUT_MS / 1000);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

// --- SNIFFER CALLBACK ---
// This catches the MAC address even if the phone doesn't "reply" to the ping
void hybrid_sniffer_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *payload = pkt->payload;

    // Search the first 20 bytes for the MAC (accounts for LLC/SNAP encapsulation)
    for (int i = 0; i < 20; i++)
    {
        if (memcmp(payload + i, target_mac, TARGET_MAC_LEN) == 0)
        {
            last_seen_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (gpio_get_level(TRIGGER_GPIO) == 0)
            {
                gpio_set_level(TRIGGER_GPIO, 1);
                status_byte |= (1<<FLAG_SNIFFER_SUCCESS);
                ESP_LOGW(WIFI_TAG, "Sniffer Catch! RSSI: %d", pkt->rx_ctrl.rssi);
            }
            return;
        }
    }
}

// --- PING CALLBACKS ---
void on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    last_seen_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    gpio_set_level(TRIGGER_GPIO, 1);
    status_byte |= (1<<FLAG_PING_SUCCESS);
    ESP_LOGI(WIFI_TAG, "Ping success! Time: %dms", (int)elapsed_time);
}

void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    // Check if we've been silent for too long
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_seen_ms > TIMEOUT_MS)
    {
        if (gpio_get_level(TRIGGER_GPIO) == 1)
        {
            gpio_set_level(TRIGGER_GPIO, 0);
            ESP_LOGE(WIFI_TAG, "User lost. Turning Mirror OFF.");
        }
    }
}

// --- INITIALIZERS ---
void start_ping_engine()
{
    ip_addr_t target_addr;
    struct in_addr addr4;
    inet_aton(PHONE_1, &addr4);
    ip_2_ip4(&target_addr)->addr = addr4.s_addr;
    target_addr.type = IPADDR_TYPE_V4;

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.interval_ms = PING_INTERVAL_MS; // Poke every 10 seconds
    ping_config.count = PING_COUNT;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = NULL,
        .cb_args = NULL};

    esp_ping_new_session(&ping_config, &cbs, &ping_handle);
    esp_ping_start(ping_handle);
    ESP_LOGI(WIFI_TAG, "Ping engine started for %s", PHONE_1);
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // Only start ping engine once we have an IP
        start_ping_engine();
    }
}
