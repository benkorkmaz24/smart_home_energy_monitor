#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"

// Modules
#include "wifi_mqtt.h"
#include "lcd_ssd1306.h"
#include "storage.h"
#include "acs712.h"

static const char *TAG_MAIN = "ENERGY_METER";

// --- Pins and Settings ---
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21

// ALARM PINS
#define LED_RED_PIN    25 
#define LED_YELLOW_PIN 27
#define BUZZER_PIN     26 

#define VOLTAGE_NOMINAL    220.0f
#define POWER_LIMIT_WATT   1300.0f 

i2c_master_bus_handle_t bus_handle;
float total_kwh = 0.0;
float unit_price_currency = 2.50; // Unit price (e.g., TL, USD)

// I2C Configuration
void i2c_custom_config(void) {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

// Initialize Alarm System
void alarm_system_init(void) {
    gpio_reset_pin(LED_RED_PIN);
    gpio_set_direction(LED_RED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_YELLOW_PIN);
    gpio_set_direction(LED_YELLOW_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    
    // Test Beep on Startup
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BUZZER_PIN, 0);
}

void app_main(void) {
    // 1. Module Initialization
    storage_init();      
    acs712_init();       
    i2c_custom_config(); 
    alarm_system_init(); 
    
    // 2. Load accumulated energy from NVS
    total_kwh = load_kwh_from_nvs(); 

    // 3. Network Connections
    wifi_mqtt_init(); 

    // 4. Display Setup
    lcd_ss1306_i2c_dev_conf(bus_handle);
    SSD1306_t dev;
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);

    char display_buffer[32];
    uint32_t last_mqtt_send_time = 0;
    static int save_counter = 0;

    while (1) {
        // --- 1. Measurement ---
        float current_rms = calculate_rms_current();
        
        // DEBUG LOG: Raw value before main loop filtering
        printf("\n------------------------------------------------\n");
        printf("[DEBUG] Raw Sensor Current: %.3f A\n", current_rms);

        // --- Safety Filter ---
        // We lowered this from 1.50A to 0.05A. 
        // Since acs712.c already handles noise, this is just a final safety net.
        if (current_rms < 1.50) current_rms = 0.0; 
        
        float power_watts = current_rms * VOLTAGE_NOMINAL;

        // LOG: Values displayed on screen
        printf("[DISPLAY] Pwr: %.2f W | Cur: %.2f A | Cost: %.2f\n", 
               power_watts, current_rms, total_kwh * unit_price_currency);
        printf("------------------------------------------------\n");

        // --- ALARM LOGIC ---
        if (power_watts > POWER_LIMIT_WATT) {
            gpio_set_level(LED_RED_PIN, 1);
            gpio_set_level(BUZZER_PIN, 1);
            gpio_set_level(LED_YELLOW_PIN, 0);
            
            ssd1306_clear_screen(&dev, false);
            ssd1306_display_text(&dev, 1, "!! WARNING !!", 12, true);
            snprintf(display_buffer, sizeof(display_buffer), "Pwr: %.0f W", power_watts);
            ssd1306_display_text(&dev, 3, display_buffer, 12, true);
        } 
        else {
            gpio_set_level(LED_RED_PIN, 0);
            gpio_set_level(BUZZER_PIN, 0);
            gpio_set_level(LED_YELLOW_PIN, 1);

            ssd1306_clear_screen(&dev, false);
            snprintf(display_buffer, sizeof(display_buffer), "Pwr: %.1f W", power_watts);
            ssd1306_display_text(&dev, 0, display_buffer, 16, false);
            snprintf(display_buffer, sizeof(display_buffer), "Cur: %.2f A", current_rms);
            ssd1306_display_text(&dev, 2, display_buffer, 16, false);
            snprintf(display_buffer, sizeof(display_buffer), "E:%.4f kWh", total_kwh);
            ssd1306_display_text(&dev, 4, display_buffer, 16, false);
            snprintf(display_buffer, sizeof(display_buffer), "Cost:%.2f", total_kwh * unit_price_currency);
            ssd1306_display_text(&dev, 6, display_buffer, 16, false);
        }

        // --- Energy Accumulation ---
        // Only accumulate significant power (> 5 Watts)
        if (power_watts > 5.0) { 
            // Calculate kWh (Sampling interval is approx. 1.1 seconds including delays)
            total_kwh += (power_watts / 1000.0f) * (1.1f / 3600.0f); 
        }

        // --- MQTT Transmission ---
        uint32_t now = esp_log_timestamp();
        if (now - last_mqtt_send_time > 10000) { 
            if (mqtt_connected) {
                // Debug log for transmission
                ESP_LOGI("MQTT_DEBUG", "Sending -> Power: %.2f W", power_watts);
                
                mqtt_send_data(TOPIC_POWER, power_watts);
                vTaskDelay(pdMS_TO_TICKS(200)); 
                mqtt_send_data(TOPIC_CURRENT, current_rms);
                vTaskDelay(pdMS_TO_TICKS(200));
                mqtt_send_data(TOPIC_COST, total_kwh * unit_price_currency);
                last_mqtt_send_time = now;
            }
        }
        
        // --- NVS Auto-Save ---
        save_counter++;
        if (save_counter >= 60) {
            save_kwh_to_nvs(total_kwh);
            save_counter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}