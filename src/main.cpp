#include <Arduino.h>
#include "Config.h"
#include "Ammonia_UV.h"
#include "BHT1750.h"
#include "MQTTManager.h"

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
// TIMERS (Non-blocking Timing)
// ====================================================================

unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 2000;

unsigned long lastLightUpdateTime = 0;
const unsigned long LIGHT_UPDATE_INTERVAL = 1000;

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
    Serial.println("      CrayLife: ESP32 Sub-system Starting    ");
    Serial.println("=============================================");

    // Initialize Ammonia Sensor + UV Relay
    ammoniaSubsystem.begin();

    // Initialize BH1750 + Grow Light
    growLight.begin();

    // Initialize MQTT
    mqttManager.begin(onMqttMessage);

    Serial.println("[STATUS] All subsystems initialized successfully.");
    Serial.println();
}

// ====================================================================
// LOOP
// ====================================================================

void loop() {

    // Maintain MQTT Connection
    mqttManager.maintain();
    mqttManager.loop();

    // Every 2 seconds
    unsigned long currentMillis = millis();

    if (currentMillis - lastUpdateTime >= UPDATE_INTERVAL) {

        lastUpdateTime = currentMillis;

        // ============================================================
        // GROW LIGHT SYSTEM (Inilipat dito para sumabay sa 2s delay)
        // ============================================================
        
        growLight.update();

        // ============================================================
        // AMMONIA SYSTEM
        // ============================================================

        ammoniaSubsystem.update();

        String ammoniaPayload =
            String(ammoniaSubsystem.getRawAmmonia());

        mqttManager.publish(
            MQTT_TOPIC_AMMONIA,
            ammoniaPayload.c_str()
        );

        // ============================================================
        // SYSTEM STATUS
        // ============================================================

        mqttManager.publish(
            MQTT_TOPIC_STATUS,
            "ONLINE"
        );
    }
}