#include "main.h"

const uint8_t target_mac_1[TARGET_MAC_LEN] = TARGET_MAC_ADDR_1;
const uint8_t target_mac_2[TARGET_MAC_LEN] = TARGET_MAC_ADDR_2;
esp_ping_handle_t ping_handle_1 = NULL;
esp_ping_handle_t ping_handle_2 = NULL;
uint32_t last_seen_ms_1 = 0;
uint32_t last_seen_ms_2 = 0;


void timeout_monitor_task(void *pvParameters)
{
    while (1)
    {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t time_1 = now - last_seen_ms_1;
        uint32_t time_2 = now - last_seen_ms_2;
        bool active_1 = time_1 < TIMEOUT_MS;
        bool active_2 = time_2 < TIMEOUT_MS;

        // If current time minus last seen time is greater than timeout
      ESP_LOGI(WIFI_TAG, "SYS ACTIVE TIMERS: ID1:%.3f|ID2:%.3f < %.3f",time_1/1e3,time_2/1e3,TIMEOUT_MS/1e3);
      if (active_1 || active_2) {
            status_byte |= FLAG_SYSTEM_ACTIVE;
            gpio_set_level(TRIGGER_GPIO, 1);
        } else {
            ESP_LOGE(WIFI_TAG, "Due Network Inactivity System Shutdown");
            status_byte &= ~FLAG_SYSTEM_ACTIVE;
            gpio_set_level(TRIGGER_GPIO, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// --- SNIFFER CALLBACK ---
// This catches the MAC address even if the phone doesn't "reply" to the ping
void hybrid_sniffer_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *payload = pkt->payload;

    // Search the first 20 bytes for the MAC (accounts for LLC/SNAP encapsulation)
    for (int i = 0; i < 20; i++)
    {
        if (memcmp(payload + i, target_mac_1, TARGET_MAC_LEN) == 0)
        {
            last_seen_ms_1 = now;
           
            return;
        }
        else if (memcmp(payload + i, target_mac_2, TARGET_MAC_LEN) == 0)
        {
            last_seen_ms_2 = now;
            return;
        }
        
    }
}

// --- PING CALLBACKS ---
void on_ping_success(esp_ping_handle_t hdl, void *args)
{   
    uint8_t id = (int)args;
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (id == 0){
        last_seen_ms_1 = now;
        ESP_LOGI(WIFI_TAG, "Ping success for Phone ID:<%d>! Time: %dms",id, (int)elapsed_time);
    }
    else if (id == 1)
    {
        last_seen_ms_2 = now;
        ESP_LOGI(WIFI_TAG, "Ping success for Phone ID:<%d>! Time: %dms",id, (int)elapsed_time);
    }
    
}


// --- INITIALIZERS ---
void start_ping_engine(const char *ip_str, uint8_t phone_id, esp_ping_handle_t *handle)
{
    ip_addr_t target_addr;
    ip4addr_aton(ip_str, ip_2_ip4(&target_addr));

    target_addr.type = IPADDR_TYPE_V4;

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.interval_ms = PING_INTERVAL_MS; 
    ping_config.count = PING_COUNT;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = NULL,
        .on_ping_end = NULL,
        .cb_args = (void *)(intptr_t)phone_id};

    esp_ping_new_session(&ping_config, &cbs, handle);
    esp_ping_start(*handle);
    ESP_LOGI(WIFI_TAG, "Ping engine started for %s", ip_str);
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
        start_ping_engine(PHONE_1, 0, &ping_handle_1);
        start_ping_engine(PHONE_2, 1, &ping_handle_2);
    }
}
