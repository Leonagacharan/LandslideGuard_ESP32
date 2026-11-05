
# Hardware Connections

## Main Controller
- ESP32 DevKit V1 (ESP-WROOM-32)

## Sensor Connections

| Component | Interface | ESP32 / Module |
|---|---|---|
| ADS1115 | I2C | SDA GPIO 21, SCL GPIO 22 |
| MPU6050 | I2C | SDA GPIO 21, SCL GPIO 22 |
| VL53L0X | I2C | SDA GPIO 21, SCL GPIO 22 |
| OLED Display | SPI | SCK 18, MOSI 23, CS 5, DC 17, RESET 16 |
| RGB LED | Digital | GPIO 25, 26, 27 |
| Buzzer | Digital | GPIO 33 |

## Soil Moisture Sensors

Three capacitive soil moisture sensors connect to the ADS1115:

- Sensor 1 → A0
- Sensor 2 → A1
- Sensor 3 → A2

## I2C Addresses

| Component | Address |
|---|---|
| ADS1115 | 0x48 |
| MPU6050 | 0x68 |
| VL53L0X | 0x29 |

## Notes
- ESP32 performs local sensor processing and TinyML inference.
- The system provides local visual and audible alerts.
- Wi-Fi-based monitoring can be integrated in a later stage.
