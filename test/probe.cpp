#include <Arduino.h>

// =====================
// PROBE PINS (FIXED)
// =====================
#define BLUE_PROBE  2    // lowest level
#define WHITE_PROBE 16   // middle level (changed from GPIO0 ❗)
#define RED_PROBE   4    // highest level

void setup() {
  Serial.begin(115200);

  // Use pull-down to avoid floating readings
  pinMode(BLUE_PROBE, INPUT_PULLDOWN);
  pinMode(WHITE_PROBE, INPUT_PULLDOWN);
  pinMode(RED_PROBE, INPUT_PULLDOWN);

  Serial.println("Water Level System Started");
}

void loop() {

  int blue  = digitalRead(BLUE_PROBE);
  int white = digitalRead(WHITE_PROBE);
  int red   = digitalRead(RED_PROBE);

  // Debug print
  Serial.print("B:");
  Serial.print(blue);
  Serial.print(" W:");
  Serial.print(white);
  Serial.print(" R:");
  Serial.println(red);

  // =====================
  // LOGIC (LEVEL SYSTEM)
  // =====================

  // 1. No probe touched
  if (blue == LOW && white == LOW && red == LOW) {
    Serial.println("Empty Tank");
  }

  // 2. Blue only
  else if (blue == HIGH && white == LOW && red == LOW) {
    Serial.println("Refill the Tank");
  }

  // 3. Blue + White
  else if (blue == HIGH && white == HIGH && red == LOW) {
    Serial.println("Almost Full Tank");
  }

  // 4. Blue + White + Red
  else if (blue == HIGH && white == HIGH && red == HIGH) {
    Serial.println("Tank is Full");
  }

  // fallback (unexpected readings)
  else {
    Serial.println("Checking Water Level...");
  }

  Serial.println("--------------------");
  delay(1000);
}