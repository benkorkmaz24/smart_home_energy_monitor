<<<<<<< HEAD
idf_component_register(
    SRCS 
        "main.c" 
        "wifi_mqtt.c" 
        "lcd_ssd1306.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "esp_driver_i2c" 
        "esp_driver_gpio" 
        "esp_adc" 
        "driver"
        "esp_wifi"       # Wi-Fi için gerekli
        "esp_event"      # Olay döngüsü için gerekli
        "nvs_flash"      # Wi-Fi ayarlarını saklamak için gerekli
        "mqtt"           # Adafruit bağlantısı için gerekli
        "esp_netif"      # İnternet katmanı için gerekli
)
=======
idf_component_register(SRCS "hello_world_main.c"
                    INCLUDE_DIRS "")
>>>>>>> origin/main
