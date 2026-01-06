#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdbool.h>

// --- Wi-Fi Ayarları ---
#define WIFI_SSID       "Bunyamin"
#define WIFI_PASS       "bnyo02bnyo"

// --- Adafruit IO Ayarları ---
#define MQTT_BROKER_URL "mqtt://io.adafruit.com"
#define MQTT_USERNAME   "BnyKorkmaz"
#define MQTT_KEY        "aio_AKNh66sZ3EUQd4BfJO66F0IrqksU"

// --- Feed Yolları (Topic) ---
// Not: Kullanıcı_Adın/feeds/feed_ismin şeklinde olmalı
#define TOPIC_POWER       "BnyKorkmaz/feeds/Power"
#define TOPIC_CURRENT     "BnyKorkmaz/feeds/Current"
#define TOPIC_COST         "BnyKorkmaz/feeds/Cost"

// Fonksiyonlar
void wifi_mqtt_init(void);
void mqtt_send_data(const char* topic, float value);

extern bool mqtt_connected; // Dışarıdan erişim için

#endif