#ifndef GROWLIGHT_H
#define GROWLIGHT_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

class GrowLight {
private:
    BH1750 lightMeter;

    int currentBrightness;
    int targetBrightness;
    int lastReadTarget;
    int stableReadCount;

public:
    GrowLight();

    void begin();
    void update();

private:
    void setBrightness(int brightness);
};

#endif