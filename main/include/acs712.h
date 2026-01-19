#ifndef ACS712_H
#define ACS712_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// --- Sensor Settings ---
// We use ADC_CHANNEL_0. If errors persist, try using the integer directly.
#define ADC_CHANNEL         ADC_CHANNEL_0 
#define ACS712_SENSITIVITY  0.066f  // For 30A model

// --- Function Definitions ---
void acs712_init(void);
float calculate_rms_current(void);

#endif