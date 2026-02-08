#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "main.h"
#include "led_strip.h"

/* ============================================================================
 * DEFINES & CONFIGURATION
 * ============================================================================ */

#define LED_STRIP_GPIO_PIN  22
#define LED_STRIP_MAX_LEDS 60

#define MIRROR_LIGHT_COLOR_ARRAY {255, 170, 33} // NEED TO BE ADJUSTED WHEN INSTALLED
#define MAX_BRIGHTNESS 50

extern led_strip_config_t strip_config;
extern led_strip_rmt_config_t rmt_config;
extern led_strip_handle_t led_strip;

/* ============================================================================
 * ENUMS & TYPES
 * ============================================================================ */


/* ============================================================================
 * STRUCTURES
 * ============================================================================ */



/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

void init_LED_params(void);

void led_strip_set_pixel_dimmed(led_strip_handle_t strip, uint32_t index, uint32_t r, uint32_t g, uint32_t b, uint16_t brightness);

void breathe_effect(void);

void led_strip_ON_sequence_soft(void);
void led_strip_OFF_sequence_soft(void);
/* ============================================================================
 * GLOBAL VARIABLES 
 * ============================================================================ */


#ifdef __cplusplus
}
#endif

#endif /* LED_CONTROL_H */
