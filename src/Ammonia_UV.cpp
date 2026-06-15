#include "Ammonia_UV.h"

// Constructor: Ipinapasa ang mga pin configurations mula sa config.h
AmmoniaUV::AmmoniaUV(int mq137Pin, int relayPin, int threshold) {
    _mq137Pin = mq137Pin;
    _relayPin = relayPin;
    _threshold = threshold;
    _rawAmmonia = 0;
    _estimatedVoltage = 0.0;
}

// Initialization ng mga pins
void AmmoniaUV::begin() {
    pinMode(_mq137Pin, INPUT);
    pinMode(_relayPin, OUTPUT);
    digitalWrite(_relayPin, HIGH); // Default state
}

// Ang pangunahing logic ng Ammonia sensor at UV Sterilizer Relay
void AmmoniaUV::update() {
    _rawAmmonia = analogRead(_mq137Pin);
    _estimatedVoltage = (_rawAmmonia / 4095.0) * 3.3;

    Serial.print("[DATA LOG] Raw ADC: ");
    Serial.print(_rawAmmonia);
    Serial.print(" | Sensor Voltage: ");
    Serial.print(_estimatedVoltage);
    Serial.print(" V");

    if (_rawAmmonia > _threshold) {
        digitalWrite(_relayPin, HIGH); 
        Serial.println(" -> [ALERT] HIGH AMMONIA DETECTED! UV Sterilizer is NOW ON.");
    } else {
        digitalWrite(_relayPin, LOW);  
        Serial.println(" -> [STATUS] Safe levels. UV Sterilizer is OFF.");
    }
}

int AmmoniaUV::getRawAmmonia() {
    return _rawAmmonia;
}

float AmmoniaUV::getVoltage() {
    return _estimatedVoltage;
}