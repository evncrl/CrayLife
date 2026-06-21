#include <Arduino.h>

#define FLOW_PIN 18

volatile int pulseCount = 0;
float flowRate;
unsigned long previousTime;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(FLOW_PIN, INPUT_PULLUP);
  
  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    countPulse,
    FALLING
  );
  
  previousTime = millis();
  
  Serial.println("==========================");
  Serial.println("  CrayLife Flow Sensor    ");
  Serial.println("       Test Mode          ");
  Serial.println("==========================");
  Serial.println("🚰 Pour water through now!");
  Serial.println("==========================");
}

void loop() {
  if (millis() - previousTime >= 1000) {

    // Calculate flow rate
    flowRate = pulseCount / 7.5;

    // Print results
    Serial.println("==========================");
    Serial.print("Pulses    : ");
    Serial.println(pulseCount);
    Serial.print("Flow Rate : ");
    Serial.print(flowRate);
    Serial.println(" L/min");

    // Status
    if (pulseCount > 0) {
      Serial.println("Status    : ✅ WORKING!");
    } else {
      Serial.println("Status    : ❌ No flow...");
    }

    Serial.println("==========================");

    // Reset
    pulseCount = 0;
    previousTime = millis();
  }
}