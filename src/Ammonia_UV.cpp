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
    
    // Default state: LOW para OFF ang Transistor at OFF ang UV sa simula
    digitalWrite(_relayPin, LOW); 
}

void AmmoniaUV::update() {
    _rawAmmonia = analogRead(_mq137Pin);
    _estimatedVoltage = (_rawAmmonia / 4095.0) * 3.3;

    Serial.print("[DATA LOG] Raw ADC: ");
    Serial.print(_rawAmmonia);
    Serial.print(" | Sensor Voltage: ");
    Serial.print(_estimatedVoltage);
    Serial.print(" V");

    if (_rawAmmonia > _threshold) {
        // ============================================================
        // TRANSISTOR LOGIC: HIGH ay magpapasindi sa Transistor (UV ON)
        // ============================================================
        digitalWrite(_relayPin, LOW); 
        Serial.println(" -> [ALERT] HIGH AMMONIA DETECTED! UV Sterilizer is NOW ON.");
    } else {
        // ============================================================
        // TRANSISTOR LOGIC: LOW ay magpapatay sa Transistor (UV OFF)
        // ============================================================
        digitalWrite(_relayPin, HIGH);  
        Serial.println(" -> [STATUS] Safe levels. UV Sterilizer is OFF.");
    }
}

int AmmoniaUV::getRawAmmonia() {
    return _rawAmmonia;
}

float AmmoniaUV::getVoltage() {
    return _estimatedVoltage;
}