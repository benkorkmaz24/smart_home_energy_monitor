#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

// Initializes the Non-Volatile Storage (NVS)
void storage_init(void);

// Saves the accumulated energy (kWh) to NVS
void save_kwh_to_nvs(float kwh);

// Loads the stored energy (kWh) value from NVS
float load_kwh_from_nvs(void);

#endif