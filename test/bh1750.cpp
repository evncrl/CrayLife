#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ====================================================================
// 📌 WI-FI & MQTT CONFIGURATIONS (Palitan ang mga detalye rito)
// ====================================================================
const char* WIFI_SSID = "PLDT_Home_61D1B";      
const char* WIFI_PASSWORD = "Pldthome123";   
const char* MQTT_SERVER = "192.168.1.172";            
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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
const int STABLE_READS_NEEDED = 10;  // ~1 second of consistent readings
const int DIMMING_STEP = 15;  // Larger step for faster dimming (0-255 in ~1.7 secs)

// ========== GLOBAL VARIABLES ==========
int currentBrightness = 0;
int targetBrightness = 0;
int lastReadTarget = -1;
int stableReadCount = 0;

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

  // Determine required brightness based on current lux
  int requiredBrightness;
  String status;

  if (lux >= BRIGHT_THRESHOLD) {
    requiredBrightness = BRIGHTNESS_OFF;
    status = "BRIGHT";
  }
  else if (lux > MEDIUM_THRESHOLD) {
    requiredBrightness = BRIGHTNESS_LOW;
    status = "HIGH AMBIENT";
  }
  else if (lux > LOW_THRESHOLD) {
    requiredBrightness = BRIGHTNESS_MEDIUM;
    status = "MEDIUM";
  }
  else {
    requiredBrightness = BRIGHTNESS_HIGH;
    status = "DARK";
  }

  Serial.print(status);
  Serial.print(" -> ");
  Serial.print(lux);
  Serial.print(" lx -> ");

  // Stability check: only change target if condition is stable
  if (requiredBrightness != lastReadTarget) {
    lastReadTarget = requiredBrightness;
    stableReadCount = 1;
    Serial.println("(stabilizing...)");
  }
  else {
    stableReadCount++;
    if (stableReadCount >= STABLE_READS_NEEDED) {
      targetBrightness = requiredBrightness;
      Serial.print("GROWLIGHT ");
      if (targetBrightness == BRIGHTNESS_OFF) {
        Serial.println("OFF");
      } else if (targetBrightness == BRIGHTNESS_LOW) {
        Serial.println("LOW");
      } else if (targetBrightness == BRIGHTNESS_MEDIUM) {
        Serial.println("MEDIUM");
      } else {
        Serial.println("HIGH");
      }
    } else {
      Serial.println("(stabilizing...)");
    }
  }

  // Smooth dimming to target
  if (currentBrightness < targetBrightness) {
    currentBrightness += DIMMING_STEP;
    if (currentBrightness > targetBrightness) {
      currentBrightness = targetBrightness;
    }
    ledcWrite(LEDC_CHANNEL, currentBrightness);
  }
  else if (currentBrightness > targetBrightness) {
    currentBrightness -= DIMMING_STEP;
    if (currentBrightness < targetBrightness) {
      currentBrightness = targetBrightness;
    }
    ledcWrite(LEDC_CHANNEL, currentBrightness);
  }

  delay(READ_DELAY_MS);
}