#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdbool.h>
#include "secrets.h" 

// --- Wi-Fi settings---
#define WIFI_SSID       SECRET_WIFI_SSID
#define WIFI_PASS       SECRET_WIFI_PASS

#define MQTT_BROKER_URL SECRET_MQTT_URL
#define MQTT_USERNAME   SECRET_MQTT_USER
#define MQTT_KEY        SECRET_MQTT_KEY

#define TOPIC_POWER     SECRET_TOPIC_POWER
#define TOPIC_CURRENT   SECRET_TOPIC_CURRENT
#define TOPIC_COST      SECRET_TOPIC_COST

void wifi_mqtt_init(void);
void mqtt_send_data(const char* topic, float value);

extern bool mqtt_connected; 

#endif