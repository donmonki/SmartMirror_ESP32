#ifndef LD2410_SENSOR_H
#define LD2410_SENSOR_H

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DEFINES & CONFIGURATION
 * ============================================================================ */

#define RX_PIN 16
#define TX_PIN 17
#define LD2410_BAUD_RATE 256000

// User defines
// #define DEBUG_MODE
#define ENHANCED_MODE
#define SENSOR_READ_INTERVAL_MS 1000  // Print data every second

#define LD2410_TAG "LD2410"

/* ============================================================================
 * STRUCTURES & TYPES
 * ============================================================================ */

typedef struct {
    bool presence_detected;
    uint16_t detected_distance_cm;
    bool moving_target_detected;
    uint16_t moving_target_distance_cm;
    uint8_t moving_target_signal;
    bool stationary_target_detected;
    uint16_t stationary_target_distance_cm;
    uint8_t stationary_target_signal;
    uint8_t light_level;
    bool output_level;
} ld2410_data_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/**
 * @brief Initialize the LD2410 sensor
 * @return true if initialization successful, false otherwise
 */
bool ld2410_init(void);

/**
 * @brief Get current sensor data
 * @return ld2410_data_t structure with latest sensor readings
 */
ld2410_data_t ld2410_get_data(void);

/**
 * @brief Check if presence is detected (moving or stationary)
 * @return true if any presence detected, false otherwise
 */
bool ld2410_is_presence_detected(void);

/**
 * @brief Print sensor data for debugging
 */
void ld2410_print_data(void);

/**
 * @brief Task function for reading sensor data
 * @param pvParameters Task parameters (unused)
 */
void ld2410_sensor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // LD2410_SENSOR_H
