#include <Arduino.h>
#include "Config.h"
#include "Ammonia_UV.h"
#include "BHT1751.h"
#include "MQTTManager.h"
// #include "IR.h"

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
// const int irPins[NUM_SENSORS] = SENSOR_PINS_INIT;
// ShelterSystem shelterSubsystem(irPins, SERVO_PIN);

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
const unsigned long ENVIRONMENT_INTERVAL = 3000;

unsigned long lastShelterTime = 0;

// ====================================================================
// DASHBOARD CACHING (for value-only updates)
// ====================================================================
int lastAmmonia = -1;
float lastVoltage = -1.0;
bool lastUVState = false;
float lastLux = -1.0;
String lastLightStatus = "";
int lastBrightness = -1;
bool lastMqttState = false;

// ====================================================================
// MQTT CALLBACK
// ====================================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    // Keep silent to prevent dashboard layout disruption
}

// ====================================================================
// HELPER: Update display line with value (using carriage return)
// ====================================================================
void printDashboardValue(const char* label, const String& value) {
    Serial.print(label);
    Serial.println(value);
}

// ====================================================================
// SETUP
// ====================================================================
void setup() {
    Serial.begin(115200);
    delay(500); // Wait for serial port to stabilize

    // ====================================================================
    // 🎨 PRINT DASHBOARD TEMPLATE ONCE (Structure only - no data yet)
    // ====================================================================
    Serial.println();
    Serial.println("=================================================");
    Serial.println("       CRAYLIFE: ESP32 MASTER SYSTEM DASHBOARD   ");
    Serial.println("=================================================");
    
    Serial.println();
    Serial.println("--- AMMONIA SUBSYSTEM ---");
    Serial.println("Ammonia RAW ADC  : [UPDATING...]"); 
    Serial.println("Sensor Voltage   : [UPDATING...]");
    Serial.println("UV Sterilizer    : [UPDATING...]");

    Serial.println();
    Serial.println("--- GROW LIGHT SUBSYSTEM ---");
    Serial.println("Ambient Light    : [UPDATING...]");
    Serial.println("Light Status     : [UPDATING...]");
    Serial.println("PWM Brightness   : [UPDATING...]");

    Serial.println();
    Serial.println("--- NETWORK STATUS ---");
    Serial.println("MQTT Connection  : [UPDATING...]");
    Serial.println("=================================================");
    Serial.println();

    // Initialize Ammonia Sensor + UV Relay
    ammoniaSubsystem.begin();

    // Initialize BT17501 + Grow Light
    growLight.begin();

    // Initialize MQTT Broker Link
    mqttManager.begin(onMqttMessage);

    Serial.println("[STATUS] System initialized. Dashboard updates starting...");
    Serial.println();
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
    // Maintain MQTT Broker Connection & Check Network Buffers Continuous
    mqttManager.maintain();
    mqttManager.loop();

    // Patuloy na patakbuhin ang light dimming logic nang walang delay block
    growLight.update();

    unsigned long currentMillis = millis();

    // ----------------------------------------------------------------
    // TASK 1: RUN SHELTER MONITORING ARRAY (Every 500ms)
    // ----------------------------------------------------------------
    // if (currentMillis - lastShelterTime >= SENSOR_POLL_INTERVAL) {
    //      lastShelterTime = currentMillis;
    //      shelterSubsystem.update();
    // }

    // ----------------------------------------------------------------
    // TASK 2: RUN ENVIRONMENTAL UPDATE & MQTT TELEMETRY (Every 3000ms)
    // ----------------------------------------------------------------
    if (currentMillis - lastEnvironmentTime >= ENVIRONMENT_INTERVAL) {
        lastEnvironmentTime = currentMillis;

        // I-update ang mga sensor evaluations sa background
        ammoniaSubsystem.update();

        // Package and ship system payloads via MQTT
        String ammoniaPayload = String(ammoniaSubsystem.getRawAmmonia());
        mqttManager.publish(MQTT_TOPIC_AMMONIA, ammoniaPayload.c_str());

        String luxPayload = String(growLight.getLux(), 1); // 1 decimal place
        mqttManager.publish(MQTT_TOPIC_LIGHT_LUX, luxPayload.c_str());

        String lightstatusPayload = growLight.getLightStatus();
        mqttManager.publish(MQTT_TOPIC_LIGHT_STATUS, lightstatusPayload.c_str());

        mqttManager.publish(MQTT_TOPIC_STATUS, "ONLINE");

        // ====================================================================
        // 📌 PURE RAW LOG VERTICAL REFRESH ENGINE (VS Code Fix)
        // ====================================================================
        // Nag-pi-print ng saktong bilang ng bagong linya para itulak pataas ang lumang dashboard.
        // Dahil dito, laging nakapako sa ilalim ng VS Code window ang pinakabagong dashboard panel.
        for (int i = 0; i < 40; i++) { Serial.println(); }

        Serial.println("=================================================");
        Serial.println("       CRAYLIFE: ESP32 MASTER SYSTEM DASHBOARD   ");
        Serial.println("=================================================");
        
        // --- AMMONIA SUBSYSTEM MONITORING ---
        Serial.print("Ammonia RAW ADC  : "); Serial.println(ammoniaSubsystem.getRawAmmonia()); 
        Serial.print("Sensor Voltage   : "); Serial.print(ammoniaSubsystem.getVoltage());    Serial.println(" V");
        Serial.print("UV Sterilizer    : "); 
        if (ammoniaSubsystem.getRawAmmonia() > AMMONIA_THRESHOLD) {
            Serial.println("[ALERT] ON - HIGH AMMONIA");
        } else {
            Serial.println("[SAFE] OFF");
        }

        Serial.println("-------------------------------------------------");

        // --- GROW LIGHT SUBSYSTEM MONITORING ---
        Serial.print("Ambient Light    : "); Serial.print(growLight.getLux(), 1);       Serial.println(" lx");
        Serial.print("Light Status     : "); Serial.println(growLight.getLightStatus()); 
        Serial.print("PWM Brightness   : "); Serial.print(growLight.getCurrentBrightness()); Serial.println(" / 255");

        Serial.println("-------------------------------------------------");

        // --- SYSTEM INVENTORY & NETWORK STATUS ---
        Serial.print("MQTT Connection  : "); 
        if (mqttManager.isConnected()) {
            Serial.println("CONNECTED");
        } else {
            Serial.println("DISCONNECTED");
        }
        Serial.println("=================================================");
    }
}