#ifndef WIFI_HANDLING_H
#define WIFI_HANDLING_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GLOBAL VARIABLES (declared in main.c)
 * ============================================================================ */

/* Target device MAC address for detection */
extern const uint8_t target_mac[TARGET_MAC_LEN];

/* Last time target device was detected (in milliseconds) */
extern uint32_t last_seen_ms_1;
extern uint32_t last_seen_ms_2;

/* Ping handle for continuous device monitoring */
extern esp_ping_handle_t ping_handle_1;
extern esp_ping_handle_t ping_handle_2;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/**
 * @brief WiFi promiscuous mode packet sniffer callback
 * @details Detects target MAC address in WiFi frames and triggers mirror
 * @param buf Pointer to the WiFi packet buffer
 * @param type Type of WiFi packet (MGMT, DATA, etc.)
 */
void hybrid_sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type);

/**
 * @brief Ping success callback
 * @details Called when a ping to the target IP succeeds
 * @param hdl Ping handle
 * @param args Callback arguments
 */
void on_ping_success(esp_ping_handle_t hdl, void *args);

/**
 * @brief Ping timeout callback
 * @details Called when a ping to the target IP times out
 * @param hdl Ping handle
 * @param args Callback arguments
 */
void on_ping_timeout(esp_ping_handle_t hdl, void *args);

/**
 * @brief Initialize and start the ping engine
 * @details Sets up periodic ping to the target iPhone IP address
 */
void start_ping_engine(void);

/**
 * @brief Timeout monitor task
 * @details Monitors device timeout and turns off mirror if device not detected
 * @param pvParameters Task parameters (unused)
 */
void timeout_monitor_task(void *pvParameters);

/**
 * @brief WiFi and IP event handler
 * @details Handles WiFi connection events and IP assignment
 * @param arg Event handler argument
 * @param event_base Event base (WIFI_EVENT or IP_EVENT)
 * @param event_id Event ID
 * @param event_data Event-specific data
 */
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_HANDLING_H */