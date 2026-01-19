#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "wifi_mqtt.h"
#include "esp_log.h"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t client;
bool mqtt_connected = false;

// MQTT Event Handler (Monitors connection status)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connection Successful!");
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected. Retrying...");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error Occurred");
            break;
        default:
            break;
    }
}

// Wi-Fi Event Handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Wi-Fi Station Started. Connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGW(TAG, "Wi-Fi Disconnected. Retrying connection...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Address Acquired: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_mqtt_init(void) {
    // 1. Initialize NVS (Required for Wi-Fi storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize Network Interface and Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            // .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Optional security threshold
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    // --- CRITICAL POWER SETTING (Brownout Prevention) ---
    // Reducing Wi-Fi TX power prevents voltage dips that freeze the I2C screen.
    // Unit is 0.25dBm. 40 * 0.25 = 10dBm (Standard is 20dBm).
    // This power level is sufficient for indoor connectivity.
    esp_wifi_set_max_tx_power(40); 
    // ----------------------------------------------------

    // 3. MQTT Configuration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_KEY,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_send_data(const char* topic, float value) {
    if (mqtt_connected) {
        char data_str[16];
        snprintf(data_str, sizeof(data_str), "%.2f", value);
        
        // Debug Log
        // ESP_LOGI("MQTT_DEBUG", "Publishing -> Topic: %s | Value: %s", topic, data_str);
        
        int msg_id = esp_mqtt_client_publish(client, topic, data_str, 0, 1, 0);
        
        if (msg_id == -1) {
             ESP_LOGE("MQTT_ERROR", "Failed to publish message!");
        }
    } else {
        ESP_LOGW("MQTT_WARN", "MQTT not connected. Data skipped.");
    }
}