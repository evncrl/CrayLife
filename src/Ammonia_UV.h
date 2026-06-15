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
    // Constructor upang i-initialize ang mga pins at threshold
    AmmoniaUV(int mq137Pin, int relayPin, int threshold);

    // I-setup ang mga pin modes
    void begin();

    // Babasahin ang sensor at ikokontrol ang relay
    void update();

    // Getters kung sakaling kailanganin mo ang data sa ibang bahagi ng code
    int getRawAmmonia();
    float getVoltage();
};

#endif // AMMONIA_UV_H