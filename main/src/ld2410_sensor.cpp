#include "ld2410_sensor.h"
#include "MyLD2410.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

// Use UART1 for the sensor (GPIO 16 RX, GPIO 17 TX on ESP32)
static const uart_port_t SENSOR_UART_PORT = UART_NUM_1;
static const uart_config_t uart_config = {
    .baud_rate = LD2410_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    .source_clk = UART_SCLK_DEFAULT
};

static MyLD2410 *sensor = NULL;
static ld2410_data_t latest_sensor_data = {0};
static uint64_t nextPrint = 0;

/* ============================================================================
 * FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

bool ld2410_init(void)
{
    ESP_LOGI(LD2410_TAG, "Initializing LD2410 sensor on UART%d", SENSOR_UART_PORT);
    
    // Configure UART
    esp_err_t ret = uart_param_config(SENSOR_UART_PORT, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(LD2410_TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Set UART pins
    ret = uart_set_pin(SENSOR_UART_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(LD2410_TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Install UART driver
    const int uart_buffer_size = (1024 * 2);
    ret = uart_driver_install(SENSOR_UART_PORT, uart_buffer_size, uart_buffer_size, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(LD2410_TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Create sensor object
    sensor = new MyLD2410(SENSOR_UART_PORT, false);
    if (sensor == nullptr) {
        ESP_LOGE(LD2410_TAG, "Failed to create MyLD2410 object");
        return false;
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    if (!sensor->begin()) {
        ESP_LOGE(LD2410_TAG, "Failed to communicate with the sensor");
        delete sensor;
        sensor = NULL;
        return false;
    }

#ifdef ENHANCED_MODE
    sensor->enhancedMode();
    ESP_LOGI(LD2410_TAG, "Enhanced mode enabled");
#else
    sensor->enhancedMode(false);
    ESP_LOGI(LD2410_TAG, "Enhanced mode disabled");
#endif

    nextPrint = 0;
    ESP_LOGI(LD2410_TAG, "LD2410 sensor initialized successfully");
    
    return true;
}

ld2410_data_t ld2410_get_data(void)
{
    if (sensor == NULL) {
        return latest_sensor_data;
    }
    
    // Check for new sensor data
    if (sensor->check() == MyLD2410::Response::DATA) {
        // Update latest data
        latest_sensor_data.presence_detected = sensor->presenceDetected();
        latest_sensor_data.detected_distance_cm = sensor->detectedDistance();
        
        latest_sensor_data.moving_target_detected = sensor->movingTargetDetected();
        if (latest_sensor_data.moving_target_detected) {
            latest_sensor_data.moving_target_signal = sensor->movingTargetSignal();
            latest_sensor_data.moving_target_distance_cm = sensor->movingTargetDistance();
        }
        
        latest_sensor_data.stationary_target_detected = sensor->stationaryTargetDetected();
        if (latest_sensor_data.stationary_target_detected) {
            latest_sensor_data.stationary_target_signal = sensor->stationaryTargetSignal();
            latest_sensor_data.stationary_target_distance_cm = sensor->stationaryTargetDistance();
        }
        
        if (sensor->inEnhancedMode() && (sensor->getFirmwareMajor() > 1)) {
            latest_sensor_data.light_level = sensor->getLightLevel();
            latest_sensor_data.output_level = sensor->getOutLevel();
        }
    }
    
    return latest_sensor_data;
}

bool ld2410_is_presence_detected(void)
{
    return latest_sensor_data.presence_detected || 
           latest_sensor_data.moving_target_detected || 
           latest_sensor_data.stationary_target_detected;
}

void ld2410_print_data(void)
{
    if (sensor == NULL) {
        return;
    }
    
    ESP_LOGI(LD2410_TAG, "Status: %s", sensor->statusString());
    
    if (latest_sensor_data.presence_detected) {
        ESP_LOGI(LD2410_TAG, "Distance: %d cm", latest_sensor_data.detected_distance_cm);
    }
    
    if (latest_sensor_data.moving_target_detected) {
        ESP_LOGI(LD2410_TAG, "MOVING = %d @ %d cm",
                 latest_sensor_data.moving_target_signal,
                 latest_sensor_data.moving_target_distance_cm);
    }

    if (latest_sensor_data.stationary_target_detected) {
        ESP_LOGI(LD2410_TAG, "STATIONARY = %d @ %d cm",
                 latest_sensor_data.stationary_target_signal,
                 latest_sensor_data.stationary_target_distance_cm);
    }
}

void ld2410_sensor_task(void *pvParameters)
{
    ESP_LOGI(LD2410_TAG, "LD2410 sensor task started");
    
    if (!ld2410_init()) {
        ESP_LOGE(LD2410_TAG, "Failed to initialize sensor, task terminating");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        // Read sensor data
        ld2410_get_data();
        
        // Print data at regular intervals
        uint64_t now = esp_timer_get_time() / 1000ULL;
        if (now > nextPrint) {
            nextPrint = now + SENSOR_READ_INTERVAL_MS;
            ld2410_print_data();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // Small delay to prevent blocking
    }
}
