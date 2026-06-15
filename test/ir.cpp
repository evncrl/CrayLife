#include <Arduino.h>
#include <ESP32Servo.h>

// Define the pins for the 5 IR Sensors
const int NUM_SENSORS = 5;
const int SENSOR_PINS[NUM_SENSORS] = {4, 16, 17, 5, 18}; 

// Define Servo Pin
const int SERVO_PIN = 32;
Servo shelterServo;

// Timing tracking variables
unsigned long simultaneousStartTime = 0; 
bool trackingActive = false;
const unsigned long REQUIRED_TRIGGER_TIME = 10000; // 10 seconds in milliseconds

void shakeShelter() {
  Serial.println("\n🚨 CRITICAL CAPTURE: 5/5 Sensors triggered for 10s! Activating shake sequence...");
  
  // Rapidly move back and forth to create a physical shaking/vibrating effect
  for (int i = 0; i < 6; i++) { 
    shelterServo.write(40);  // Quick tilt up
    delay(150);              // Short delay for sudden mechanical snapping
    shelterServo.write(0);   // Drop back down flat
    delay(150);
  }
  
  Serial.println("🔄 Shaking sequence complete. Evacuation forced.");
  
  // 3-second grace period cooldown to let crayfish completely run away 
  // and prevent immediate re-triggering while they scramble out
  delay(3000); 
}

void setup() {
  Serial.begin(115200);
  
  // Initialize all 5 sensor pins as inputs using a loop
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }
  
  // Allocate hardware timers for ESP32 Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  shelterServo.setPeriodHertz(50);
  shelterServo.attach(SERVO_PIN, 500, 2400);
  
  // Start flat/at rest
  shelterServo.write(0);
  
  Serial.println("--- 5-Sensor Array and Shake System Online ---");
}

void loop() {
  bool allSensorsTriggered = true;
  int triggeredCount = 0;

  // Read all 5 sensors
  for (int i = 0; i < NUM_SENSORS; i++) {
    // Remember: HW-201 outputs LOW (0) when it detects an obstacle
    if (digitalRead(SENSOR_PINS[i]) == HIGH) {
      allSensorsTriggered = false; // At least one sensor is clear
    } else {
      triggeredCount++;
    }
  }

  // Handle the 10-second simultaneous timer logic
  if (allSensorsTriggered) {
    if (!trackingActive) {
      // This is the exact millisecond all 5 sensors became blocked together
      simultaneousStartTime = millis();
      trackingActive = true;
      Serial.println("⚠️ All 5 sensors are blocked! Starting 10-second countdown...");
    } else {
      // Calculate how many seconds have elapsed since the simultaneous block started
      unsigned long elapsedSeconds = (millis() - simultaneousStartTime) / 1000;
      Serial.printf("⏳ Holding... Sensors blocked for %lu / 10 seconds\n", elapsedSeconds);
      
      // Check if 10 seconds have passed
      if (millis() - simultaneousStartTime >= REQUIRED_TRIGGER_TIME) {
        shakeShelter();
        trackingActive = false; // Reset tracking after execution
      }
    }
  } else {
    // If even a single sensor goes clear, completely reset the clock
    if (trackingActive) {
      Serial.println("🟢 Sensor array cleared or interrupted. Resetting countdown clock.");
      trackingActive = false;
    }
    
    // Safety check: Keep shelter completely flat while waiting
    shelterServo.write(0);
    
    // Optional debug visual: Prints how many are currently triggered
    if (triggeredCount > 0) {
      Serial.printf("ℹ️ Partial occupancy: Only %d out of 5 sensors triggered.\n", triggeredCount);
    }
  }

  delay(500); // Check the array twice every second to preserve loop stability
}