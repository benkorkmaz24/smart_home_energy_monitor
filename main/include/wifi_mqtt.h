#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdbool.h>

// --- Wi-Fi Settings ---
#define WIFI_SSID       "Bunyamin"
#define WIFI_PASS       "bnyo02bnyo"

// --- Adafruit IO Settings ---
#define MQTT_BROKER_URL "mqtt://io.adafruit.com"
#define MQTT_USERNAME   "BnyKorkmaz"
#define MQTT_KEY        "aio_iNgR89jJUrzmfzBZ3MzaPxroEKkf"

// --- Feed Paths (Topics) ---
// Note: Must be in the format "Username/feeds/feed_name" (Case sensitive!)
#define TOPIC_POWER       "BnyKorkmaz/feeds/power"
#define TOPIC_CURRENT     "BnyKorkmaz/feeds/current"
#define TOPIC_COST        "BnyKorkmaz/feeds/cost"

// Function Declarations
void wifi_mqtt_init(void);
void mqtt_send_data(const char* topic, float value);

extern bool mqtt_connected; // Flag for external access to connection status

#endif