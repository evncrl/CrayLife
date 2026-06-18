#include "BHT1750.h"
#include "Config.h"

GrowLight::GrowLight() {
    currentBrightness = 0;
    targetBrightness = 0;
    lastReadTarget = -1;
    stableReadCount = 0;
}

void GrowLight::begin() {

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

void GrowLight::update() {

    float lux = lightMeter.readLightLevel();

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

            if (targetBrightness == BRIGHTNESS_OFF)
                Serial.println("OFF");
            else if (targetBrightness == BRIGHTNESS_LOW)
                Serial.println("LOW");
            else if (targetBrightness == BRIGHTNESS_MEDIUM)
                Serial.println("MEDIUM");
            else
                Serial.println("HIGH");
        }
        else {
            Serial.println("(stabilizing...)");
        }
    }

    if (currentBrightness < targetBrightness) {

        currentBrightness += DIMMING_STEP;

        if (currentBrightness > targetBrightness)
            currentBrightness = targetBrightness;

        setBrightness(currentBrightness);
    }
    else if (currentBrightness > targetBrightness) {

        currentBrightness -= DIMMING_STEP;

        if (currentBrightness < targetBrightness)
            currentBrightness = targetBrightness;

        setBrightness(currentBrightness);
    }
}

void GrowLight::setBrightness(int brightness) {
    ledcWrite(LEDC_CHANNEL, brightness);
}