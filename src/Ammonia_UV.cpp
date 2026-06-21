#include "Ammonia_UV.h"

AmmoniaUV::AmmoniaUV(int mq137Pin, int relayPin, int threshold) {
    _mq137Pin = mq137Pin;
    _relayPin = relayPin;
    _threshold = threshold;
    _rawAmmonia = 0;
    _estimatedVoltage = 0.0;
}

// Initialization function to set pin modes and default states
void AmmoniaUV::begin() {
    pinMode(_mq137Pin, INPUT);
    pinMode(_relayPin, OUTPUT);
    
    // Default state: LOW pin for the relay (UV OFF)
    digitalWrite(_relayPin, LOW); 
}

void AmmoniaUV::update() {
    _rawAmmonia = analogRead(_mq137Pin);
    _estimatedVoltage = (_rawAmmonia / 4095.0) * 3.3;

    if (_rawAmmonia > _threshold) {
        digitalWrite(_relayPin, LOW); 
    } else {
        digitalWrite(_relayPin, HIGH);  
    }
}

int AmmoniaUV::getRawAmmonia() {
    return _rawAmmonia;
}

float AmmoniaUV::getVoltage() {
    return _estimatedVoltage;
}