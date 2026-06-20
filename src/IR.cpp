#include "IR.h"

// ====================================================================
// 📌 CLASS CONSTRUCTOR (5 Arguments)
// ====================================================================
IR::IR(int numSensors, const int* sensorPins, int servoPin, unsigned long requiredTriggerTime, MQTTManager& mqttManager) {
    _numSensors = numSensors;
    _sensorPins = sensorPins;
    _servoPin = servoPin;
    _requiredTriggerTime = requiredTriggerTime;
    _mqttManager = &mqttManager;
    
    // Initialize internal state tracking variables
    _lastTriggeredCount = -1;
    _simultaneousStartTime = 0;
    _trackingActive = false;
}

// ====================================================================
// 📌 HARDWARE INITIALIZATION
// ====================================================================
void IR::begin() {
    for (int i = 0; i < _numSensors; i++) {
        pinMode(_sensorPins[i], INPUT);
    }
    
    // Allocate hardware PWM timers for ESP32 Servo stability
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    _shelterServo.setPeriodHertz(50);
    _shelterServo.attach(_servoPin, 500, 2400);
    _shelterServo.write(0); // Start flat
}

// ====================================================================
// 📌 SHAKE MOTIONS LOGIC
// ====================================================================
void IR::shakeShelter() {
    _mqttManager->publish(MQTT_TOPIC_STATUS, "SHAKING"); // Notify broker instantly
    
    // Physical shake pattern loop
    for (int i = 0; i < 5; i++) { 
        _shelterServo.write(180);  // Quick tilt up to full range
        delay(150);              
        _shelterServo.write(0);    // Drop flat
        delay(150);
    }
    
    _mqttManager->publish(MQTT_TOPIC_STATUS, "IDLE"); // Reset state
    delay(3000); // Cooldown grace period
}

// ====================================================================
// 📌 MAIN RUNTIME PROCESSING 
// ====================================================================
void IR::update() {
    bool allSensorsTriggered = true;
    int triggeredCount = 0;

    // Evaluate array states (LOW = Object Detected)
    for (int i = 0; i < _numSensors; i++) {
        if (digitalRead(_sensorPins[i]) == HIGH) {
            allSensorsTriggered = false; 
        } else {
            triggeredCount++;
        }
    }

    // Publish telemetry changes dynamically using light sensor fallbacks as a pathway
    if (triggeredCount != _lastTriggeredCount) {
        String countStr = String(triggeredCount);
        _mqttManager->publish(MQTT_TOPIC_LIGHT_LUX, countStr.c_str()); 
        _lastTriggeredCount = triggeredCount;
    }

    // Timer logic operations
    if (allSensorsTriggered) {
        if (!_trackingActive) {
            _simultaneousStartTime = millis();
            _trackingActive = true;
            _mqttManager->publish(MQTT_TOPIC_STATUS, "PENDING_EVACUATION");
        } else {
            if (millis() - _simultaneousStartTime >= _requiredTriggerTime) {
                shakeShelter();
                _trackingActive = false; // Reset cycle tracking state
            }
        }
    } else {
        // Fallback break handling
        if (_trackingActive) {
            _mqttManager->publish(MQTT_TOPIC_STATUS, "IDLE");
            _trackingActive = false;
        }
        _shelterServo.write(0); // Lock low flat state safety default
    }
}

// ====================================================================
// 📌 GETTERS
// ====================================================================
int IR::getLastTriggeredCount() { return _lastTriggeredCount; }
bool IR::isTrackingActive() { return _trackingActive; }