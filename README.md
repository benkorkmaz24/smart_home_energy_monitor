|# ⚡ Smart Home Energy Monitor (ESP32 IoT)

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.shields.io/badge/platform-ESP32-orange.svg) ![Language](https://img.shields.io/badge/language-C-green)

A real-time, IoT-enabled Smart Energy Monitor built with **ESP32** and **ACS712**. This system measures AC current, calculates power consumption (RMS), and tracks cumulative energy cost. Data is visualized locally on an OLED display and transmitted remotely via **MQTT (Adafruit IO)**.

## 🚀 Key Features

* **True RMS Measurement:** Calculates Root Mean Square current over 150ms sampling windows for accurate AC readings.
* **Smart Noise Gate:** Implements a software threshold (`~0.05A`) to eliminate sensor ghost noise when idle.
* **Data Persistence (NVS):** Saves accumulated energy (kWh) and cost to ESP32's Non-Volatile Storage every minute. Data is preserved even after power loss.
* **IoT Connectivity:** Transmits Power, Current, and Cost data to **Adafruit IO Cloud** via MQTT protocol.
* **Brownout Protection:** Optimized Wi-Fi TX power (`10dBm`) to prevent voltage dips and system freezes.
* **Safety Alarm System:** Visual (LED) and Audio (Buzzer) alerts when power exceeds **1300W**.

## 🛠️ Hardware Requirements

| Component | Description |
|-----------|-------------|
| **ESP32 DevKit V1** | Main Microcontroller (Wi-Fi + Bluetooth) |
| **ACS712 (30A)**    | Hall Effect Current Sensor |
| **SSD1306 OLED**    | 0.96" I2C Display (128x64) |
| **Buzzer**          | Active Buzzer for Alarm |
| **LEDs**            | Red (Alarm), Yellow (Status) |
| **Resistors**       | 220Ω or 330Ω for LEDs |

## 🔌 Pin Configuration

| ESP32 Pin | Component Pin | Function |
|-----------|---------------|----------|
| **GPIO 36 (VP)** | ACS712 OUT | Analog Input (ADC1_CH0) |
| **GPIO 21**  | OLED SDA | I2C Data |
| **GPIO 22**  | OLED SCL | I2C Clock |
| **GPIO 25**  | Red LED | Alarm Indicator (High Power) |
| **GPIO 26**  | Buzzer | Audible Alarm |
| **GPIO 27**  | Yellow LED | Normal Status Indicator |
| **5V / VIN** | VCC | Power Supply |
| **GND**      | GND | Ground |

## ⚙️ Installation & Setup

1.  **Clone the Repository:**
    ```bash
    git clone (https://github.com/benkorkmaz24/smart_home_energy_monitor.git)
    cd smart_home_energy_monitor
    ```

2.  **Configure Credentials:**
    * Rename `main/include/secrets_example.h` to `secrets.h`.
    * Enter your Wi-Fi SSID, Password, and Adafruit IO Key in `secrets.h`.
    *(Note: `secrets.h` is ignored by Git for security)*

3.  **Build and Flash:**
    * Open the project in **VS Code** with **ESP-IDF Extension**.
    * Build the project.
    * Flash to your ESP32 board.

## 📊 Software Architecture

The system operates on a non-blocking loop architecture:
1.  **Sampling:** Reads raw ADC values for 150ms to determine Peak-to-Peak voltage.
2.  **Filtering:** Applies a noise gate threshold to filter out idle sensor noise.
3.  **Calculation:** Converts voltage to RMS Current -> Power (Watts) -> Accumulated Energy (kWh).
4.  **Logic:** Triggers Alarm if `Power > 1300W`.
5.  **Transmission:** Publishes MQTT data every 10 seconds.
6.  **Storage:** Commits data to NVS Flash every 60 seconds.

## ⚠️ Disclaimer
This project involves measuring AC mains voltage. **Extreme caution** must be taken when working with 220V/110V. Ensure proper isolation and never touch the circuit while connected to mains power.

---
*Developed by Bünyamin KORKMAZ*
