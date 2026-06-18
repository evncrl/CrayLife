#include "BHT1751.h"

// Constructor: Set initial tracking defaults
GrowLight::GrowLight() {
    _currentBrightness = BRIGHTNESS_OFF;
    _targetBrightness = BRIGHTNESS_OFF;
    _lastReadTarget = -1;
    _stableReadCount = 0;
    _currentStatus = "UNKNOWN";
}

// Initialization task
void GrowLight::begin() {
    // PWM Setup utilizing modern ledc parameters
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(GROWLIGHT_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, BRIGHTNESS_OFF);

    // Initializing BH1750 on defined I2C pins
    Wire.begin(SDA_PIN, SCL_PIN);
    _lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

// Main execution loop method called inside master loop
void GrowLight::update() {
    float lux = getLux();
    int requiredBrightness;

    // Evaluate light condition threshold maps
    if (lux >= BRIGHT_THRESHOLD) {
        requiredBrightness = BRIGHTNESS_OFF;
        _currentStatus = "BRIGHT";
    }
    else if (lux > MEDIUM_THRESHOLD) {
        // Adjusts to target your High Ambient configuration
        requiredBrightness = 77; // Mapping target match from initial draft
        _currentStatus = "HIGH AMBIENT";
    }
    else if (lux > LOW_THRESHOLD) {
        requiredBrightness = BRIGHTNESS_MEDIUM;
        _currentStatus = "MEDIUM";
    }
    else if (lux > DARK_THRESHOLD) {
        requiredBrightness = BRIGHTNESS_LOW;
        _currentStatus = "LOW";
    }
    else {
        requiredBrightness = BRIGHTNESS_HIGH;
        _currentStatus = "DARK";
    }

    // Check for sensor data stabilization
    if (requiredBrightness != _lastReadTarget) {
        _lastReadTarget = requiredBrightness;
        _stableReadCount = 1;
    }
    else {
        _stableReadCount++;
        if (_stableReadCount >= STABLE_READS_NEEDED) {
            _targetBrightness = requiredBrightness;
        }
    }

    // Smooth dimming handling engine
    if (_currentBrightness < _targetBrightness) {
        _currentBrightness += DIMMING_STEP;
        if (_currentBrightness > _targetBrightness) {
            _currentBrightness = _targetBrightness;
        }
        ledcWrite(LEDC_CHANNEL, _currentBrightness);
    }
    else if (_currentBrightness > _targetBrightness) {
        _currentBrightness -= DIMMING_STEP;
        if (_currentBrightness < _targetBrightness) {
            _currentBrightness = _targetBrightness;
        }
        ledcWrite(LEDC_CHANNEL, _currentBrightness);
    }
}

float GrowLight::getLux() {
    return _lightMeter.readLightLevel();
}

int GrowLight::getCurrentBrightness() {
    return _currentBrightness;
}

String GrowLight::getLightStatus() {
    return _currentStatus;
}