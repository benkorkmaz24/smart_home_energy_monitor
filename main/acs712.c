#include "acs712.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <math.h>

static adc_oneshot_unit_handle_t adc1_handle;

void acs712_init(void) {
    // Initialize the ADC unit
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configure the ADC channel
    // 12dB attenuation allows reading up to approx. 3.3V
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));
    
    ESP_LOGI("ACS712", "Sensor initialized.");
}

float calculate_rms_current(void) {
    int raw_val = 0;
    int max_val = 0;
    int min_val = 4095; // Max possible 12-bit ADC value
    
    // Scan signal for 150ms to find Peak-to-Peak values
    uint32_t start_time = esp_log_timestamp();
    while ((esp_log_timestamp() - start_time) < 150) { 
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_val);
        if (raw_val > max_val) max_val = raw_val;
        if (raw_val < min_val) min_val = raw_val;
        esp_rom_delay_us(50); 
    }

    int peak_to_peak = max_val - min_val;

    // --- NOISE FILTER CONFIGURATION ---
    // Log for calibration:
    // This value represents the "Noise Score". Idle noise was around 45-55.
    ESP_LOGI("CALIBRATION_LOG", "Noise Score (P2P): %d", peak_to_peak);

    // --- THRESHOLD: 65 ---
    // Values below 65 (approx. 1.35A or ~300W) are considered noise and ignored.
    // Active loads (like Blender) will exceed this and be displayed.
    if (peak_to_peak < 65) return 0.0f; 

    // Convert to Voltage
    float v_pp = (peak_to_peak * 3.3f) / 4095.0f;
    // Calculate RMS Voltage (assuming sine wave)
    float v_rms = (v_pp / 2.0f) * 0.707f; 
    
    // Calculate Current
    // 1.15f is a compensation multiplier for ADC attenuation loss.
    float current = (v_rms / ACS712_SENSITIVITY) * 1.15f;

    return current;
}