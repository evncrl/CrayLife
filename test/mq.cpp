#include <Arduino.h>
#include <WiFi.h>        
#include <PubSubClient.h> 

// ====================================================================
// 📌 WI-FI & MQTT CONFIGURATIONS (Palitan ang mga detalye rito)
// ====================================================================
const char* WIFI_SSID = "PLDT_HOME_61D1B";      
const char* WIFI_PASSWORD = "Pldthome123";   
const char* MQTT_SERVER = "192.168.1.173";            
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const int MQ137_PIN = 34;  
const int RELAY_PIN = 26;  

const int AMMONIA_THRESHOLD = 1000; 

void setup() {
  Serial.begin(115200);
  
  pinMode(MQ137_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, HIGH); 
  
  Serial.println("=============================================");
  Serial.println("      CrayLife: ESP32 Sub-system Active     ");
  Serial.println("=============================================");
  Serial.println("[STATUS] ESP32 Pins configured (High-Level Relay Mode).");
  Serial.println("[REMINDER] MQ137 requires 24-48 hours of continuous burn-in time.");
  Serial.println("---------------------------------------------");
}

void loop() {
  int rawAmmonia = analogRead(MQ137_PIN);
  
  float estimatedVoltage = (rawAmmonia / 4095.0) * 3.3;
  
  Serial.print("[DATA LOG] Raw ADC: ");
  Serial.print(rawAmmonia);
  Serial.print(" | Sensor Voltage: ");
  Serial.print(estimatedVoltage);
  Serial.print(" V");
  
  if (rawAmmonia > AMMONIA_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH); 
    Serial.println(" -> [ALERT] HIGH AMMONIA DETECTED! UV Sterilizer is NOW ON.");
  } else {
    digitalWrite(RELAY_PIN, LOW);  
    Serial.println(" -> [STATUS] Safe levels. UV Sterilizer is OFF.");
  }
  
  delay(2000); 
}