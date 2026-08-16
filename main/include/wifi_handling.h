#ifndef WIFI_HANDLING_H
#define WIFI_HANDLING_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t target_mac_1[TARGET_MAC_LEN];
extern const uint8_t target_mac_2[TARGET_MAC_LEN];

extern uint32_t last_seen_ms_1;
extern uint32_t last_seen_ms_2;

extern esp_ping_handle_t ping_handle_1;
extern esp_ping_handle_t ping_handle_2;

void hybrid_sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type);
void on_ping_success(esp_ping_handle_t hdl, void *args);
void on_ping_timeout(esp_ping_handle_t hdl, void *args);
void start_ping_engine(const char *ip_str, uint8_t phone_id, esp_ping_handle_t *handle);
void timeout_monitor_task(void *pvParameters);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_HANDLING_H */