#include <Arduino.h>
#include "Config.h"
#include "Ammonia_UV.h"
#include "BHT1750.h"
#include "MQTTManager.h"
#include "IR.h"

// ====================================================================
// SUBSYSTEM INSTANCES
// ====================================================================

// Ammonia + UV Control
AmmoniaUV ammoniaSubsystem(
    MQ137_PIN,
    RELAY_PIN,
    AMMONIA_THRESHOLD
);

// Grow Light Control
GrowLight growLight;

// 🆕 Shelter Dynamic Structural Controller
const int irPins[NUM_SENSORS] = SENSOR_PINS_INIT;
ShelterSystem shelterSubsystem(irPins, SERVO_PIN);

// MQTT Manager
MQTTManager mqttManager(
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_SERVER,
    MQTT_PORT,
    MQTT_CLIENT_ID,
    MQTT_TOPIC_CONTROL
);

// ====================================================================
// TIMERS (Non-blocking Timing Tasks)
// ====================================================================
unsigned long lastEnvironmentTime = 0;
const unsigned long ENVIRONMENT_INTERVAL = 2000;

unsigned long lastShelterTime = 0;

// ====================================================================
// MQTT CALLBACK
// ====================================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String message = "";

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("[MQTT RX] Topic: ");
    Serial.print(topic);
    Serial.print(" | Payload: ");
    Serial.println(message);

    // Future manual controls:
    // UV Sterilizer
    // Grow Light Override
}

// ====================================================================
// SETUP
// ====================================================================
void setup() {
    Serial.begin(115200);

    Serial.println();
    Serial.println("=============================================");
    Serial.println("    CrayLife: ESP32 Master System Online     ");
    Serial.println("=============================================");

    // Initialize Ammonia Sensor + UV Relay
    ammoniaSubsystem.begin();

    // Initialize BH1750 + Grow Light
    growLight.begin();

    // 🆕 Initialize Modular Shelter Subsystem
    shelterSubsystem.begin();

    // Initialize MQTT Broker Link
    mqttManager.begin(onMqttMessage);

    Serial.println("[STATUS] All modular subsystems bound successfully.");
    Serial.println();
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
    // Maintain MQTT Broker Connection & Check Network Buffers Continuous
    mqttManager.maintain();
    mqttManager.loop();

    unsigned long currentMillis = millis();

    // ----------------------------------------------------------------
    // TASK 1: RUN SHELTER MONITORING ARRAY (Every 500ms)
    // ----------------------------------------------------------------
    if (currentMillis - lastShelterTime >= SENSOR_POLL_INTERVAL) {
        lastShelterTime = currentMillis;
        shelterSubsystem.update();
    }

    // ----------------------------------------------------------------
    // TASK 2: RUN ENVIRONMENTAL UPDATE & MQTT TELEMETRY (Every 2000ms)
    // ----------------------------------------------------------------
    if (currentMillis - lastEnvironmentTime >= ENVIRONMENT_INTERVAL) {
        lastEnvironmentTime = currentMillis;

        // Run automated grow light scaling logic
        growLight.update();

        // Run ammonia threshold comparison evaluations
        ammoniaSubsystem.update();

        // Package and ship system payloads
        String ammoniaPayload = String(ammoniaSubsystem.getRawAmmonia());
        mqttManager.publish(MQTT_TOPIC_AMMONIA, ammoniaPayload.c_str());
        mqttManager.publish(MQTT_TOPIC_STATUS, "ONLINE");
    }
}