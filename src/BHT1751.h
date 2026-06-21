#ifndef BHT1751_H
#define BHT1751_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include "config.h" // Gives access to thresholds, pins, and PWM settings

class GrowLight {
private:
    BH1750 _lightMeter;
    
    // Smooth Dimming & Stability Variables
    int _currentBrightness;
    int _targetBrightness;
    int _lastReadTarget;
    int _stableReadCount;
    String _currentStatus;

public:
    // Constructor
    GrowLight();

    // Setup hardware pins, I2C, and PWM initial configurations
    void begin();

    // Core automation logic to scale brightness smoothly
    void update();

    // Getters to fetch operational values for external telemetry/MQTT
    float getLux();
    int getCurrentBrightness();

    String getLightStatus();
};

#endif // BHT1751_H