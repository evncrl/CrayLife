#ifndef AMMONIA_UV_H
#define AMMONIA_UV_H

#include <Arduino.h>

class AmmoniaUV {
private:
    int _mq137Pin;
    int _relayPin;
    int _threshold;
    int _rawAmmonia;
    float _estimatedVoltage;

public:
    AmmoniaUV(int mq137Pin, int relayPin, int threshold);

    void begin();

    void update();

    int getRawAmmonia();
    float getVoltage();
};

#endif // AMMONIA_UV_H