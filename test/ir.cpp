#include "IR.h"
#include "Config.h"

ShelterSystem::ShelterSystem(const int pins[5], int servoPin) {
    for (int i = 0; i < 5; i++) {
        _sensorPins[i] = pins[i];
    }
    _servoPin = servoPin;
    _simultaneousStartTime = 0;
    _trackingActive = false;
}

void ShelterSystem::begin() {
    // Initialize all 5 sensor pins as inputs
    for (int i = 0; i < 5; i++) {
        pinMode(_sensorPins[i], INPUT);
    }

    // Allocate hardware timers safely for the ESP32 Servo environment
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    _shelterServo.setPeriodHertz(50);
    _shelterServo.attach(_servoPin, 500, 2400);
    _shelterServo.write(0); // Start flat/at rest
    
    Serial.println("[STATUS] Shelter 5-Sensor Array and Servo initialized.");
}

void ShelterSystem::shakeShelter() {
    Serial.println("\n🚨 CRITICAL CAPTURE: 5/5 Sensors triggered for 10s! Activating shake sequence...");

    // Rapidly move back and forth to create a physical shaking/vibrating effect
    for (int i = 0; i < 6; i++) {
        _shelterServo.write(40); // Quick tilt up
        delay(150);
        _shelterServo.write(0);  // Drop back down flat
        delay(150);
    }

    Serial.println("🔄 Shaking sequence complete. Evacuation forced.");

    // 3-second grace period cooldown to let crayfish completely scramble out
    delay(3000);
}

void ShelterSystem::update() {
    bool allSensorsTriggered = true;
    int triggeredCount = 0;

    // Read all 5 sensors
    for (int i = 0; i < 5; i++) {
        // HW-201 outputs LOW (0) when it detects an obstacle
        if (digitalRead(_sensorPins[i]) == HIGH) {
            allSensorsTriggered = false; // At least one sensor is clear
        } else {
            triggeredCount++;
        }
    }

    // Handle the 10-second simultaneous timer logic
    if (allSensorsTriggered) {
        if (!_trackingActive) {
            _simultaneousStartTime = millis();
            _trackingActive = true;
            Serial.println("⚠️ All 5 sensors are blocked! Starting 10-second countdown...");
        } else {
            unsigned long elapsedSeconds = (millis() - _simultaneousStartTime) / 1000;
            Serial.printf("⏳ Holding... Sensors blocked for %lu / 10 seconds\n", elapsedSeconds);

            // Check if threshold milestone has been hit
            if (millis() - _simultaneousStartTime >= REQUIRED_TRIGGER_TIME) {
                shakeShelter();
                _trackingActive = false; // Reset tracking after execution
                forceClockSync();       // Wipe runtime delay lag from state
            }
        }
    } else {
        // If even a single sensor goes clear, completely reset the clock
        if (_trackingActive) {
            Serial.println("🟢 Sensor array cleared or interrupted. Resetting countdown clock.");
            _trackingActive = false;
        }

        // Safety check: Keep shelter completely flat while waiting
        _shelterServo.write(0);

        // Optional debug visual
        if (triggeredCount > 0) {
            Serial.printf("ℹ️ Partial occupancy: Only %d out of 5 sensors triggered.\n", triggeredCount);
        }
    }
}

void ShelterSystem::forceClockSync() {
    _simultaneousStartTime = millis();
}