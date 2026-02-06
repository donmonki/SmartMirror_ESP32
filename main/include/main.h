#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "ping/ping_sock.h"
#include "driver/gpio.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"

/* ============================================================================
 * DEFINES & CONFIGURATION
 * ============================================================================ */

/* Application Configuration */
#define APP_NAME                "SmartMirror"
#define APP_VERSION             "1.0.0"
#define WIFI_TAG                "WIFI_CHECKER"

/* Hardware Configuration */
#define TRIGGER_GPIO            23

/* WiFi Configuration */
#define WIFI_SSID               "ICO-1C792D"
#define WIFI_PASS               "RydWuetbids4"

/* iPhone Detection Configuration */
#define PHONE_1          "192.168.0.111"
#define PHONE_2          "192.168.0.197"
#define TIMEOUT_MS              36e6  /* 10min before turning off mirror */

/* Ping Configuration */
#define PING_INTERVAL_MS        30000   /* Ping every 30 seconds */
#define PING_COUNT              ESP_PING_COUNT_INFINITE

/* Task Configuration */
#define TASK_PRIORITY_HIGH      (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_NORMAL    (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_LOW       (tskIDLE_PRIORITY + 1)

#define TASK_STACK_SIZE_SMALL   (2048)
#define TASK_STACK_SIZE_MEDIUM  (4096)
#define TASK_STACK_SIZE_LARGE   (8192)

/* WiFi MAC Address Detection */
#define TARGET_MAC_LEN          6
#define TARGET_MAC_ADDR_1         {0x8A, 0xD4, 0xD7, 0x47, 0xC9, 0x62}
#define TARGET_MAC_ADDR_2         {0x9e, 0x37, 0x32, 0x80, 0x33, 0x75}

#define MAC_SEARCH_WINDOW       20  /* Search first 20 bytes for MAC address */


/* ============================================================================
 * ENUMS & TYPES
 * ============================================================================ */
/* STATUS BYTE BIT REPRESENTATION */
typedef enum{
    FLAG_SYSTEM_ACTIVE = 0,
    FLAG_PING_SUCCESS = 1,
    FLAG_SNIFFER_SUCCESS = 2,


}Status_bit_rep_t;


/* ============================================================================
 * STRUCTURES
 * ============================================================================ */



/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

void app_main(void);

/* ============================================================================
 * GLOBAL VARIABLES 
 * ============================================================================ */

extern uint8_t status_byte;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
