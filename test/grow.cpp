#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

// ========== LIGHT THRESHOLDS ==========
const float DARK_THRESHOLD = 1.5;      // Very dark
const float LOW_THRESHOLD = 100.0;     // Low light
const float MEDIUM_THRESHOLD = 500.0;  // Medium light
const float BRIGHT_THRESHOLD = 1000.0; // Very bright

// ========== GROW LIGHT BRIGHTNESS ==========
const int BRIGHTNESS_OFF = 0;       // OFF
const int BRIGHTNESS_LOW = 38;      // 15%
const int BRIGHTNESS_MEDIUM = 76;   // 30%
const int BRIGHTNESS_HIGH = 255;    // 100%

// ========== PIN CONNECTIONS ==========
const int GROWLIGHT_PIN = 23;  // MOSFET Gate -> GPIO23
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ========== PWM SETTINGS ==========
const int LEDC_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;

// ========== TIMING ==========
const int READ_DELAY_MS = 100;

// ========== GLOBAL VARIABLES ==========
int currentBrightness = 0;
int targetBrightness = 0;

void setup() {

  Serial.begin(115200);

  // PWM Setup
  ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(GROWLIGHT_PIN, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, BRIGHTNESS_OFF);

  // BH1750 Setup
  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  Serial.println("================================");
  Serial.println("AUTO GROW LIGHT SYSTEM");
  Serial.println("OFF    : >1000 lux");
  Serial.println("LOW    : 500-1000 lux");
  Serial.println("MEDIUM : 100-500 lux");
  Serial.println("HIGH   : <100 lux");
  Serial.println("================================");
}

void loop() {

  float lux = lightMeter.readLightLevel();

  // Determine target brightness
  if (lux >= BRIGHT_THRESHOLD) {

    targetBrightness = BRIGHTNESS_OFF;

    Serial.print("BRIGHT -> ");
    Serial.print(lux);
    Serial.println(" lx -> GROWLIGHT OFF");
  }

  else if (lux > MEDIUM_THRESHOLD) {

    targetBrightness = BRIGHTNESS_LOW;

    Serial.print("HIGH AMBIENT -> ");
    Serial.print(lux);
    Serial.println(" lx -> GROWLIGHT LOW");
  }

  else if (lux > LOW_THRESHOLD) {

    targetBrightness = BRIGHTNESS_MEDIUM;

    Serial.print("MEDIUM -> ");
    Serial.print(lux);
    Serial.println(" lx -> GROWLIGHT MEDIUM");
  }

  else {

    targetBrightness = BRIGHTNESS_HIGH;

    Serial.print("DARK -> ");
    Serial.print(lux);
    Serial.println(" lx -> GROWLIGHT HIGH");
  }

  // Smooth Dimming
  if (currentBrightness < targetBrightness) {
    currentBrightness++;
    ledcWrite(LEDC_CHANNEL, currentBrightness);
  }
  else if (currentBrightness > targetBrightness) {
    currentBrightness--;
    ledcWrite(LEDC_CHANNEL, currentBrightness);
  }

  delay(READ_DELAY_MS);
}
