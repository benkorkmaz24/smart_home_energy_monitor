#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "lcd_ssd1306.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"
#include "wifi_mqtt.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG_MAIN = "ENERGY_METER";

// --- Pin ve Sensör Tanımlamaları ---
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define ADC_CHANNEL       ADC_CHANNEL_0 
#define ACS712_SENSITIVITY 0.066f       
#define VOLTAGE_NOMINAL    220.0f

// --- Global Değişkenler ---
i2c_master_bus_handle_t bus_handle;
adc_oneshot_unit_handle_t adc1_handle;
float total_kwh = 0.0;
float unit_price_tl = 2.50; 

// --- Fonksiyon Protokolleri ---
void i2c_custom_config(void);
void adc_init_custom(void);
float calculate_rms_current(void);
void save_kwh_to_nvs(float kwh);
float load_kwh_from_nvs(void);

// --- NVS (Hafıza) Fonksiyonları ---
void save_kwh_to_nvs(float kwh) {
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_i32(my_handle, "kwh_int", (int32_t)(kwh * 10000));
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

float load_kwh_from_nvs() {
    nvs_handle_t my_handle;
    int32_t kwh_int = 0;
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_i32(my_handle, "kwh_int", &kwh_int);
        nvs_close(my_handle);
    }
    return (float)kwh_int / 10000.0f;
}

// --- Ana Uygulama ---
void app_main(void) {
    ESP_LOGI(TAG_MAIN, "Sistem baslatiliyor...");
    
    // 1. NVS Başlatma ve Eski Veriyi Yükleme
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    total_kwh = load_kwh_from_nvs(); // Hafızadaki veriyi çek
    ESP_LOGI(TAG_MAIN, "Hafizadan yuklenen enerji: %.4f kWh", total_kwh);

    // 2. Donanım Başlatmaları
    i2c_custom_config();
    adc_init_custom();
    wifi_mqtt_init(); 

    ESP_LOGI("SYSTEM", "Internet bekleniyor...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // 3. Ekran Başlatma
    lcd_ss1306_i2c_dev_conf(bus_handle);
    SSD1306_t dev;
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);

    char display_buffer[32];
    uint32_t last_mqtt_send_time = 0;
    static int save_counter = 0;

    while (1) {
        // 4. Ölçüm ve Filtreleme
        float current_rms = calculate_rms_current();
        if (current_rms < 1.50) current_rms = 0.0; // Gürültü temizliği
        
        float power_watts = current_rms * VOLTAGE_NOMINAL;

        // 5. Enerji Hesabı (Zamanlama hassasiyeti için yaklasik 1 sn periyot)
        if (power_watts > 10.0) {
            total_kwh += (power_watts / 1000.0f) * (1.1f / 3600.0f); 
        }
        float total_cost = total_kwh * unit_price_tl;

        // 6. Adafruit IO Hız Sınırlayıcı (Her 10 saniyede bir gönder)
        uint32_t now = esp_log_timestamp();
        if (now - last_mqtt_send_time > 10000) { 
            if (mqtt_connected) {
                mqtt_send_data(TOPIC_POWER, power_watts);
                vTaskDelay(pdMS_TO_TICKS(500)); // İki paket arasına nefes payı
                mqtt_send_data(TOPIC_CURRENT, current_rms);
                vTaskDelay(pdMS_TO_TICKS(500));
                mqtt_send_data(TOPIC_COST, total_cost);
                
                last_mqtt_send_time = now;
                ESP_LOGI(TAG_MAIN, "Veriler Adafruit'e gonderildi.");
            }
        }

        // 7. OLED ve Seri Çıktı
        ssd1306_clear_screen(&dev, false);
        if (power_watts > 1000.0) {
            ssd1306_clear_screen(&dev, true); // Ters renk uyarı
            ssd1306_display_text(&dev, 1, "!! OVERLOAD !!", 14, false);
        } else {
            snprintf(display_buffer, sizeof(display_buffer), "Guc: %.1f W", power_watts);
            ssd1306_display_text(&dev, 0, display_buffer, 16, false);
            snprintf(display_buffer, sizeof(display_buffer), "Akim: %.2f A", current_rms);
            ssd1306_display_text(&dev, 2, display_buffer, 16, false);
            snprintf(display_buffer, sizeof(display_buffer), "Enerji:%.4f kWh", total_kwh);
            ssd1306_display_text(&dev, 4, display_buffer, 18, false);
            snprintf(display_buffer, sizeof(display_buffer), "Maliyet:%.2f TL", total_cost);
            ssd1306_display_text(&dev, 6, display_buffer, 16, false);
        }
        
        printf("Akim: %.2f A | Guc: %.1f W | Toplam: %.4f kWh | Maliyet: %.2f TL\n", 
                        current_rms, power_watts, total_kwh, total_cost);

        // 8. NVS Kaydı (Dakikada bir)
        save_counter++;
        if (save_counter >= 60) {
            save_kwh_to_nvs(total_kwh);
            save_counter = 0;
            ESP_LOGI(TAG_MAIN, "Hafizaya kaydedildi.");
        }

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

// --- Kayıp Fonksiyonların Tanımları (Artık Burada!) ---

float calculate_rms_current(void) {
    int raw_val = 0;
    int max_val = 0;
    int min_val = 4095;
    
    // 100ms boyunca tepe noktalarını yakala
    uint32_t start_time = esp_log_timestamp();
    while ((esp_log_timestamp() - start_time) < 100) { 
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_val);
        if (raw_val > max_val) max_val = raw_val;
        if (raw_val < min_val) min_val = raw_val;
        esp_rom_delay_us(100);
    }

    int peak_to_peak = max_val - min_val;

    // Sensör gürültü eşiği (Hassas ayar)
    if (peak_to_peak < 35) return 0.0f; 

    float v_pp = (peak_to_peak * 3.3f) / 4095.0f;
    float v_rms = v_pp / 2.828f; // Vpp / (2 * sqrt(2))
    return (v_rms / ACS712_SENSITIVITY);
}

void adc_init_custom(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));
}

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