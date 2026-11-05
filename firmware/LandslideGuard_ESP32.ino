
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "landslide_model.h"

// =====================================================
// LANDSLIDEGUARD ESP32 - UPDATED TINYML INTEGRATION
// =====================================================

// I2C
#define SDA_PIN 21
#define SCL_PIN 22

// SPI OLED
#define OLED_SCK   18
#define OLED_MOSI  23
#define OLED_CS     5
#define OLED_DC    17
#define OLED_RESET 16

// RGB LED and buzzer
#define LED_R 25
#define LED_G 26
#define LED_B 27
#define BUZZER 33

// MPU6050
#define MPU_ADDR 0x68

// ADS1115 moisture channels
const uint8_t MOISTURE_CHANNELS[3] = {0, 1, 2};

// =====================================================
// MOISTURE CALIBRATION REFERENCES
// Values collected experimentally
// =====================================================

const float DRY_REF[3] = {
  14192.0f,
  14361.0f,
  14635.0f
};

const float WET_REF[3] = {
  3955.0f,
  3689.0f,
  4075.0f
};

const uint8_t MOISTURE_SAMPLE_COUNT = 10;

// =====================================================
// SENSOR OBJECTS
// =====================================================

Adafruit_ADS1115 ads;
Adafruit_VL53L0X tof;

Adafruit_SSD1306 display(
  128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS
);

Eloquent::ML::Port::DecisionTree model;

// =====================================================
// SENSOR STATUS
// =====================================================

bool adsReady = false;
bool tofReady = false;
bool oledReady = false;
bool mpuReady = false;

bool firstSample = true;
bool distanceReferenceSet = false;

// =====================================================
// PREVIOUS SENSOR VALUES
// =====================================================

float previousAverage = 0.0f;
float previousTilt = 0.0f;
float previousVibration = 0.0f;
float previousDisplacement = 0.0f;

float distanceReference = 0.0f;

unsigned long previousMillis = 0;
unsigned long previousSampleMillis = 0;

const unsigned long SAMPLE_INTERVAL_MS = 1000;

// =====================================================
// MOISTURE SCALING
// =====================================================

float moistureScale(float raw, float dryRef, float wetRef) {

  if (dryRef == wetRef) {
    return NAN;
  }

  float value =
    100.0f * (dryRef - raw) / (dryRef - wetRef);

  return constrain(value, 0.0f, 100.0f);
}

// =====================================================
// READ AND AVERAGE MOISTURE SENSOR
// =====================================================

float readMoistureRaw(uint8_t channel, bool &valid) {

  long total = 0;

  for (int i = 0; i < MOISTURE_SAMPLE_COUNT; i++) {

    int16_t reading = ads.readADC_SingleEnded(channel);

    if (reading < 0) {
      valid = false;
      return NAN;
    }

    total += reading;
    delay(5);
  }

  return (float)total / MOISTURE_SAMPLE_COUNT;
}

// =====================================================
// MPU6050 REGISTER WRITE
// =====================================================

bool writeMPURegister(uint8_t reg, uint8_t value) {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);

  return Wire.endTransmission() == 0;
}

// =====================================================
// MPU6050 DATA READING
// =====================================================

bool readMPU(float &tiltDeg, float &vibrationG) {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)MPU_ADDR, 14, true) != 14) {
    return false;
  }

  int16_t axRaw = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t ayRaw = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t azRaw = (int16_t)((Wire.read() << 8) | Wire.read());

  // Discard temperature and gyroscope readings
  for (int i = 0; i < 8; i++) {
    Wire.read();
  }

  float ax = axRaw / 16384.0f;
  float ay = ayRaw / 16384.0f;
  float az = azRaw / 16384.0f;

  tiltDeg = atan2f(
    ax,
    sqrtf(ay * ay + az * az)
  ) * 57.2957795f;

  // Approximate dynamic acceleration magnitude
  vibrationG = fabsf(
    sqrtf(ax * ax + ay * ay + az * az) - 1.0f
  );

  return true;
}

// =====================================================
// RISK LABELS
// Model class map:
// 0 CRITICAL
// 1 HIGH
// 2 LOW
// 3 MODERATE
// =====================================================

const char* riskName(int prediction) {

  switch (prediction) {

    case 0: return "CRITICAL";
    case 1: return "HIGH";
    case 2: return "LOW";
    case 3: return "MODERATE";

    default: return "UNKNOWN";
  }
}

// =====================================================
// RGB LED AND BUZZER OUTPUTS
// =====================================================

void setRiskOutputs(int prediction) {

  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(BUZZER, LOW);

  switch (prediction) {

    case 2:
      // LOW: Green
      digitalWrite(LED_G, HIGH);
      break;

    case 3:
      // MODERATE: Yellow
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, HIGH);
      break;

    case 1:
      // HIGH: Red
      digitalWrite(LED_R, HIGH);
      break;

    case 0:
      // CRITICAL: Red + Buzzer
      digitalWrite(LED_R, HIGH);
      digitalWrite(BUZZER, HIGH);
      break;

    default:
      // Invalid prediction: all outputs OFF
      break;
  }
}

// =====================================================
// OLED DISPLAY
// =====================================================

void updateOLED(
  float avgMoisture,
  float moistureRange,
  float tilt,
  float distance,
  int prediction,
  bool moistureValid
) {

  if (!oledReady) {
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("LANDSLIDEGUARD");
  display.println("----------------");

  display.print("Moisture: ");

  if (moistureValid) {
    display.print(avgMoisture, 1);
    display.println("%");
  } else {
    display.println("SENSOR ERROR");
  }

  display.print("Range: ");
  display.println(
    moistureValid ? moistureRange : -1.0f,
    1
  );

  display.print("Tilt: ");
  display.print(mpuReady ? tilt : 0.0f, 1);
  display.println(" deg");

  display.print("Distance: ");

  if (distance >= 0.0f) {
    display.print(distance, 0);
    display.println(" mm");
  } else {
    display.println("N/A");
  }

  display.print("Model: ");
  display.println(riskName(prediction));

  display.println("Experimental only");

  display.display();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);
  delay(800);

  // Configure output pins
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(BUZZER, LOW);

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println();
  Serial.println("================================");
  Serial.println(" LANDSLIDEGUARD ESP32");
  Serial.println(" UPDATED TINYML INTEGRATION");
  Serial.println("================================");

  // ---------------------------------------------------
  // ADS1115 INITIALIZATION
  // ---------------------------------------------------

  adsReady = ads.begin(0x48);

  if (adsReady) {
    ads.setGain(GAIN_ONE);
    Serial.println("ADS1115: OK");
  } else {
    Serial.println("ERROR: ADS1115 NOT DETECTED");
  }

  // ---------------------------------------------------
  // VL53L0X INITIALIZATION
  // ---------------------------------------------------

  tofReady = tof.begin();

  Serial.println(
    tofReady ? "VL53L0X: OK" : "WARNING: VL53L0X UNAVAILABLE"
  );

  // ---------------------------------------------------
  // OLED INITIALIZATION
  // ---------------------------------------------------

  SPI.begin(
    OLED_SCK,
    -1,
    OLED_MOSI,
    OLED_CS
  );

  oledReady = display.begin(SSD1306_SWITCHCAPVCC);

  Serial.println(
    oledReady ? "OLED: OK" : "WARNING: OLED UNAVAILABLE"
  );

  // ---------------------------------------------------
  // MPU6050 INITIALIZATION
  // ---------------------------------------------------

  mpuReady = writeMPURegister(0x6B, 0x00);

  if (mpuReady) {

    writeMPURegister(0x1C, 0x00); // +/-2g
    writeMPURegister(0x1B, 0x00); // +/-250 deg/s
  }

  Serial.println(
    mpuReady ? "MPU6050: OK" : "WARNING: MPU6050 UNAVAILABLE"
  );

  // ---------------------------------------------------
  // CALIBRATION INFORMATION
  // ---------------------------------------------------

  Serial.println();
  Serial.println("MOISTURE CALIBRATION REFERENCES");

  for (int i = 0; i < 3; i++) {

    Serial.print("Sensor ");
    Serial.print(i + 1);

    Serial.print(" | Dry: ");
    Serial.print(DRY_REF[i], 0);

    Serial.print(" | Wet: ");
    Serial.println(WET_REF[i], 0);
  }

  Serial.println("--------------------------------");
  Serial.println("Initialization complete.");
  Serial.println("Model predictions are experimental.");
  Serial.println("Rainfall input remains a placeholder.");
  Serial.println("--------------------------------");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  if (millis() - previousMillis < SAMPLE_INTERVAL_MS) {
    return;
  }

  previousMillis = millis();

  // ===================================================
  // 1. MOISTURE SENSOR READINGS
  // ===================================================

  int16_t raw[3] = {0, 0, 0};

  float moisture[3] = {
    NAN, NAN, NAN
  };

  bool moistureValid = adsReady;

  if (adsReady) {

    for (int i = 0; i < 3; i++) {

      bool channelValid = true;

      float rawAverage = readMoistureRaw(
        MOISTURE_CHANNELS[i],
        channelValid
      );

      if (!channelValid || isnan(rawAverage)) {

        moistureValid = false;
        break;
      }

      raw[i] = (int16_t)roundf(rawAverage);

      moisture[i] = moistureScale(
        rawAverage,
        DRY_REF[i],
        WET_REF[i]
      );
    }
  }

  // ===================================================
  // 2. MOISTURE STATISTICS
  // ===================================================

  float avgMoisture = 0.0f;
  float moistureRange = 0.0f;
  float maxMoisture = 0.0f;

  if (moistureValid) {

    float minMoisture = moisture[0];
    maxMoisture = moisture[0];

    float totalMoisture = 0.0f;

    for (int i = 0; i < 3; i++) {

      totalMoisture += moisture[i];

      if (moisture[i] < minMoisture) {
        minMoisture = moisture[i];
      }

      if (moisture[i] > maxMoisture) {
        maxMoisture = moisture[i];
      }
    }

    avgMoisture = totalMoisture / 3.0f;

    moistureRange = maxMoisture - minMoisture;
  }

  // ===================================================
  // 3. MPU6050 READINGS
  // ===================================================

  float tilt = 0.0f;
  float vibration = 0.0f;

  bool motionValid = mpuReady &&
    readMPU(tilt, vibration);

  // ===================================================
  // 4. VL53L0X DISTANCE READING
  // ===================================================

  float distance = -1.0f;

  if (tofReady) {

    VL53L0X_RangingMeasurementData_t measurement;

    tof.rangingTest(&measurement, false);

    if (
      measurement.RangeStatus == 0 &&
      measurement.RangeMilliMeter > 0
    ) {
      distance = (float)measurement.RangeMilliMeter;
    }
  }

  // ===================================================
  // 5. DISPLACEMENT CALCULATION
  // ===================================================

  if (distance >= 0.0f && !distanceReferenceSet) {

    distanceReference = distance;
    distanceReferenceSet = true;
  }

  float displacement = 0.0f;

  if (distance >= 0.0f && distanceReferenceSet) {

    displacement = distance - distanceReference;
  }

  // ===================================================
  // 6. RATE CALCULATIONS
  // ===================================================

  unsigned long now = millis();

  float dt = (previousSampleMillis == 0)
    ? 0.0f
    : (now - previousSampleMillis) / 1000.0f;

  float moistureRate = 0.0f;
  float tiltRate = 0.0f;
  float displacementRate = 0.0f;
  float vibrationRate = 0.0f;

  if (!firstSample && dt > 0.0f) {

    if (moistureValid) {

      moistureRate =
        (avgMoisture - previousAverage) / dt;
    }

    if (motionValid) {

      float tiltDelta = tilt - previousTilt;

      if (tiltDelta > 180.0f) {
        tiltDelta -= 360.0f;
      }

      if (tiltDelta < -180.0f) {
        tiltDelta += 360.0f;
      }

      tiltRate = tiltDelta / dt;

      vibrationRate =
        (vibration - previousVibration) / dt;
    }

    if (distance >= 0.0f && distanceReferenceSet) {

      displacementRate =
        (displacement - previousDisplacement) / dt;
    }
  }

  // ===================================================
  // 7. RAINFALL PLACEHOLDER
  // Existing model still expects this feature.
  // This is NOT a measured rainfall value.
  // ===================================================

  float rainfall = 5.0f;
  float rainfallRate = 0.0f;

  // ===================================================
  // 8. SERIAL MONITOR SENSOR OUTPUT
  // ===================================================

  Serial.println();
  Serial.println("========== SENSOR DATA ==========");

  for (int i = 0; i < 3; i++) {

    Serial.print("Moisture ");
    Serial.print(i + 1);

    Serial.print(" | Raw ADC: ");
    Serial.print(raw[i]);

    Serial.print(" | Moisture: ");

    if (moistureValid) {

      Serial.print(moisture[i], 1);
      Serial.println("%");

    } else {

      Serial.println("INVALID");
    }
  }

  // ===================================================
  // 9. INVALID MOISTURE SAFETY CHECK
  // ===================================================

  if (!moistureValid) {

    Serial.println("ERROR: Invalid ADS1115 readings.");
    Serial.println("TinyML prediction skipped.");

    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
    digitalWrite(BUZZER, LOW);

    if (oledReady) {

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);

      display.println("LANDSLIDEGUARD");
      display.println();
      display.println("MOISTURE INPUT");
      display.println("INVALID");
      display.println("Check ADS1115");

      display.display();
    }

    previousSampleMillis = now;
    firstSample = false;

    return;
  }

  // ===================================================
  // 10. SENSOR STATISTICS
  // ===================================================

  Serial.println("--------------------------------");

  Serial.print("Average moisture: ");
  Serial.print(avgMoisture, 1);
  Serial.println("%");

  Serial.print("Maximum moisture: ");
  Serial.print(maxMoisture, 1);
  Serial.println("%");

  Serial.print("Moisture range: ");
  Serial.print(moistureRange, 1);
  Serial.println("%");

  Serial.print("Tilt: ");
  Serial.println(motionValid ? tilt : -999.0f, 2);

  Serial.print("Vibration: ");
  Serial.println(motionValid ? vibration : -1.0f, 3);

  Serial.print("ToF distance (mm): ");
  Serial.println(distance, 1);

  Serial.print("Relative displacement (mm): ");
  Serial.println(displacement, 1);

  // ===================================================
  // 11. TINYML FEATURE ARRAY
  // Preserve existing exported model's 11-feature order.
  // ===================================================

  float features[11] = {

    rainfall,
    avgMoisture,
    moistureRange,

    motionValid ? fabsf(tilt) : 0.0f,

    distance >= 0.0f
      ? fabsf(displacement)
      : 0.0f,

    motionValid ? vibration : 0.0f,

    moistureRate,

    motionValid ? tiltRate : 0.0f,

    displacementRate,

    motionValid ? vibrationRate : 0.0f,

    rainfallRate
  };

  // ===================================================
  // 12. MODEL INPUT VALIDATION
  // ===================================================

  bool modelInputsAvailable =
    motionValid &&
    distance >= 0.0f &&
    distanceReferenceSet;

  int prediction = -1;

  if (modelInputsAvailable) {

    prediction = model.predict(features);
  }

  // ===================================================
  // 13. TINYML PREDICTION OUTPUT
  // ===================================================

  Serial.println("--------------------------------");

  Serial.print("Predicted class: ");

  if (prediction >= 0) {

    Serial.println(riskName(prediction));

  } else {

    Serial.println("NOT AVAILABLE - check MPU/ToF");
  }

  Serial.println(
    "WARNING: Experimental model output only."
  );

  // ===================================================
  // 14. RGB LED AND BUZZER
  // ===================================================

  setRiskOutputs(prediction);

  // ===================================================
  // 15. OLED UPDATE
  // ===================================================
  // ===================================================
// HIGH MOISTURE SAFETY ALERT
// Trigger buzzer when average moisture exceeds 95%
// ===================================================

if (avgMoisture > 95.0f) {
  digitalWrite(BUZZER, HIGH);
  Serial.println("!!! HIGH MOISTURE ALERT !!!");
  Serial.println("BUZZER ACTIVATED - Moisture above 95%");
}

  updateOLED(
    avgMoisture,
    moistureRange,
    tilt,
    distance,
    prediction,
    moistureValid
  );

  // ===================================================
  // 16. UPDATE PREVIOUS VALUES
  // ===================================================

  previousAverage = avgMoisture;

  if (motionValid) {

    previousTilt = tilt;
    previousVibration = vibration;
  }

  if (distance >= 0.0f) {

    previousDisplacement = displacement;
  }

  previousSampleMillis = now;
  firstSample = false;
}
