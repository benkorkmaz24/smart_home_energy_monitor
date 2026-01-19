#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "STORAGE";

// Initializes the Non-Volatile Storage (NVS) system
void storage_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        // Retry nvs_flash_init
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized successfully.");
}

void save_kwh_to_nvs(float kwh) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    
    if (err == ESP_OK) {
        // Store float value as int32 to prevent precision loss (multiply by 10000)
        // Example: 1.2345 kWh becomes 12345
        int32_t stored_val = (int32_t)(kwh * 10000);
        
        err = nvs_set_i32(my_handle, "kwh_int", stored_val);
        
        if (err == ESP_OK) {
            err = nvs_commit(my_handle);
            if (err == ESP_OK) {
                // Log only occasionally or on demand to avoid spamming, 
                // but here it is useful for debugging.
                // ESP_LOGI(TAG, "Data saved to NVS: %.4f kWh", kwh);
            }
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG, "Error opening NVS handle!");
    }
}

float load_kwh_from_nvs(void) {
    nvs_handle_t my_handle;
    int32_t kwh_int = 0;
    
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_i32(my_handle, "kwh_int", &kwh_int);
        nvs_close(my_handle);
        
        float restored_val = (float)kwh_int / 10000.0f;
        ESP_LOGI(TAG, "Data loaded from NVS: %.4f kWh", restored_val);
        return restored_val;
    }
    
    ESP_LOGW(TAG, "No saved data found (or read error). Starting from 0.00 kWh");
    return 0.0f;
}