#ifndef IR_H
#define IR_H

#include <Arduino.h>
#include "config.h"
#include "MQTTManager.h"

class IR {
private:
    // Hardware configuration variables
    int _numSensors;
    const int* _sensorPins;
    int _servoPin;
    unsigned long _requiredTriggerTime;

    // References to centralized MQTT infrastructure
    MQTTManager* _mqttManager;

    // Internal state tracking
    int _lastTriggeredCount;
    unsigned long _simultaneousStartTime;
    bool _trackingActive;
    
    // Non-blocking shake state machine
    int _shakePhase;              // 0=idle, 1-5=shake movements, 6=cooldown
    unsigned long _shakeStartTime;
    const int SHAKE_MOVE_DURATION = 150;  // ms per servo movement
    const int SHAKE_COOLDOWN_DURATION = 3000;  // ms cooldown after shake

public:
    // This constructor matches your 5-argument main.cpp call perfectly
    IR(int numSensors, const int* sensorPins, int servoPin, unsigned long requiredTriggerTime, MQTTManager& mqttManager);

    // Initializer equivalent to code inside setup()
    void begin();

    // Core sensor evaluation logic processing for loop()
    void update();

    // Shake routine execution
    void shakeShelter();

    // Expose current data or states if needed externally
    int getLastTriggeredCount();
    bool isTrackingActive();
};

#endif // IR_H