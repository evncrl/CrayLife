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
    _shakePhase = 0;
    _shakeStartTime = 0;
}

// ====================================================================
// 📌 HARDWARE INITIALIZATION
// ====================================================================
void IR::begin() {
    for (int i = 0; i < _numSensors; i++) {
        pinMode(_sensorPins[i], INPUT);
    }
    
    ledcSetup(SERVO_LEDC_CHANNEL, 50, 14);  
    ledcAttachPin(_servoPin, SERVO_LEDC_CHANNEL);
    ledcWrite(SERVO_LEDC_CHANNEL, 410);  
}

// ====================================================================
// 📌 NON-BLOCKING SHAKE STATE MACHINE (Prevents grow light power sag)
// ====================================================================
void IR::shakeShelter() {
    // Entry point: start shake sequence
    _shakePhase = 1;
    _shakeStartTime = millis();
    _mqttManager->publish(MQTT_TOPIC_STATUS, "SHAKING");
}

// ====================================================================
// 📌 MAIN RUNTIME PROCESSING (Non-blocking state machine)
// ====================================================================
void IR::update() {
    unsigned long currentMillis = millis();
    
    // ====================================================================
    // 1. ACTIVE SHAKE PROCESSING (Takes absolute priority)
    // ====================================================================
    if (_shakePhase > 0) {
        unsigned long elapsedSincePhaseStart = currentMillis - _shakeStartTime;
        
        // Phases 1-10: 5 shake cycles (each = 180deg up, then 0deg down)
        if (_shakePhase <= 10) {
            if (elapsedSincePhaseStart < SHAKE_MOVE_DURATION) {
                // Odd phases = move to 180, even phases = move to 0
                int pulseWidth = (_shakePhase % 2 == 1) ? 1966 : 410;  
                ledcWrite(SERVO_LEDC_CHANNEL, pulseWidth);
            } else {
                _shakePhase++;
                _shakeStartTime = currentMillis;
            }
        }
        // Phase 11: Cooldown period
        else if (_shakePhase == 11) {
            ledcWrite(SERVO_LEDC_CHANNEL, 410);  // Return to 0°
            if (elapsedSincePhaseStart >= SHAKE_COOLDOWN_DURATION) {
                _shakePhase = 0;  // End shake sequence completely
                _mqttManager->publish(MQTT_TOPIC_STATUS, "IDLE");
            }
        }
        return;  // CRITICAL: Exit immediately. Do not run any code below while shaking!
    }
    
    // ====================================================================
    // 2. NORMAL SENSOR EVALUATION (Only runs when IDLE)
    // ====================================================================
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

    // Publish shelter occupancy changes on a dedicated topic
    if (triggeredCount != _lastTriggeredCount) {
        String countStr = String(triggeredCount);
        _mqttManager->publish(MQTT_TOPIC_SHELTER_COUNT, countStr.c_str()); 
        _lastTriggeredCount = triggeredCount;
    }

    // ====================================================================
    // 3. COUNTDOWN AND SAFETY LOGIC
    // ====================================================================
    if (allSensorsTriggered) {
        if (!_trackingActive) {
            _simultaneousStartTime = currentMillis;
            _trackingActive = true;
            _mqttManager->publish(MQTT_TOPIC_STATUS, "PENDING_EVACUATION");
        } else {
            if (currentMillis - _simultaneousStartTime >= _requiredTriggerTime) {
                shakeShelter();          // Sets _shakePhase = 1
                _trackingActive = false; // Reset tracking
                return;                  // CRITICAL: Exit instantly so safety default below isn't hit!
            }
        }
    } else {
        // Fallback break handling: If countdown was running but someone left, reset to IDLE
        if (_trackingActive) {
            _mqttManager->publish(MQTT_TOPIC_STATUS, "IDLE");
            _trackingActive = false;
        }
        ledcWrite(SERVO_LEDC_CHANNEL, 410);  // Keep locked at 0° while resting safely
    }
}

// ====================================================================
// 📌 GETTERS
// ====================================================================
int IR::getLastTriggeredCount() { return _lastTriggeredCount; }
bool IR::isTrackingActive() { return _trackingActive; }