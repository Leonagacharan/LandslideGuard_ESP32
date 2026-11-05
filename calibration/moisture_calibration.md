# Soil Moisture Sensor Calibration

Project: LandslideGuard_ESP32

## Calibration method

Relative moisture index calculated using:

moisture (%) = ((dryRaw - currentRaw) / (dryRaw - wetRaw)) * 100

Values are constrained between 0 and 100%.

## Dry soil reference

| Sensor | Raw ADC |
|--------|---------|
| A0 | 14192 |
| A1 | 14361 |
| A2 | 14635 |

## Wet soil reference

| Sensor | Raw ADC |
|--------|---------|
| A0 | 3955 |
| A1 | 3689 |
| A2 | 4075 |

## Open-air observations

| Sensor | Raw ADC | Voltage |
|--------|---------|---------|
| A0 | 14750 | 1.8438 V |
| A1 | 14952 | 1.8690 V |
| A2 | 14832 | 1.8540 V |

## Notes

- Three capacitive soil moisture sensors are connected through ADS1115.
- Sensors are intended for installation at different soil depths.
- Calibration values are based on the current bench setup.
- Moisture percentages represent relative calibrated indices, not laboratory-measured volumetric water content.
- Calibration should be rechecked after permanent installation.
