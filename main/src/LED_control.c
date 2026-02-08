#include "LED_control.h"
#include "main.h"
#include "esp_log.h"

led_strip_handle_t led_strip;

uint16_t color_array[3] = MIRROR_LIGHT_COLOR_ARRAY;

void init_LED_params(void)
{

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,
        .max_leds = LED_STRIP_MAX_LEDS, // Adjust this based on your physical strip length
        .led_model = LED_MODEL_WS2812,
        // Use the helper macro for GRB format as required by the WS2812B datasheet [cite: 106]
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false, // Set to true only if using an external inverting level shifter
        }};

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz (1 tick = 100ns)
        .mem_block_symbols = 64,           // Increase if driving > 100 LEDs
        .flags.with_dma = false,           // Set true for very long strips to save CPU
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    ESP_LOGI(LED_TAG, "WS2812B Initialized on GPIO %d", LED_STRIP_GPIO_PIN);
}

void led_strip_set_pixel_dimmed(led_strip_handle_t strip, uint32_t index, uint32_t r, uint32_t g, uint32_t b, uint16_t brightness){
    if (brightness > 255) brightness = 255;

    // Scale each component
    uint32_t red   = (r * brightness) / 255;
    uint32_t green = (g * brightness) / 255;
    uint32_t blue  = (b * brightness) / 255;

    // Push to the driver (which handles the GRB ordering for WS2812B)
    led_strip_set_pixel(strip, index, red, green, blue);

}

void led_strip_ON_sequence_soft(void){

    // NEED TO ADJUST LED NUMBERING WHEN FINALIZED
    for (int b = 0; b <= MAX_BRIGHTNESS; b++) {
            for (int i = 0; i < LED_STRIP_MAX_LEDS; i++) {
                led_strip_set_pixel_dimmed(led_strip, i, color_array[0], color_array[1], color_array[2], b);
            }
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(40));
        }

}


void led_strip_OFF_sequence_soft(void){
    
     // NEED TO ADJUST LED NUMBERING WHEN FINALIZED
    for (int b = MAX_BRIGHTNESS; b <= 0; b--) {
            for (int i = 0; i < LED_STRIP_MAX_LEDS; i++) {
                led_strip_set_pixel_dimmed(led_strip, i, color_array[0], color_array[1], color_array[2], b);
            }
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(40));
        }

}
