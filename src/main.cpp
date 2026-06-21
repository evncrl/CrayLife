#include <Arduino.h>
#include "config.h"
#include "Ammonia_UV.h"
#include "BHT1751.h"
#include "MQTTManager.h"
#include "IR.h"

const int irPins[NUM_SENSORS] = SENSOR_PINS_INIT;
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

// 🆕 Shelter Dynamic Structural Controller

IR shelterSubsystem(
    NUM_SENSORS, 
    irPins, 
    SERVO_PIN, 
    REQUIRED_TRIGGER_TIME, 
    mqttManager
);

// ====================================================================
// TIMERS (Non-blocking Timing Tasks)
// ====================================================================
unsigned long lastEnvironmentTime = 0;
const unsigned long ENVIRONMENT_INTERVAL = 1000;

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
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    // Remote Command Manual Override Handling
    if (message == "SHAKE") {
        shelterSubsystem.shakeShelter();
    }
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
    Serial.println("--- SHELTER SUBSYSTEM ---");
    Serial.println("Occupied Count   : [UPDATING...]");
    Serial.println("Timer Countdown  : [UPDATING...]");

    Serial.println();
    Serial.println("--- NETWORK STATUS ---");
    Serial.println("MQTT Connection  : [UPDATING...]");
    Serial.println("=================================================");
    Serial.println();

    // Initialize Ammonia Sensor + UV Relay
    ammoniaSubsystem.begin();

    // Initialize BT17501 + Grow Light
    growLight.begin();

    // Initialize Shelter Monitoring Array
    shelterSubsystem.begin();

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

    growLight.update();
    
    // CRITICAL FIX: Run the shelter subsystem unthrottled every loop!
    // Its internal state machine handles its own timing loops safely.
    shelterSubsystem.update(); 

    unsigned long currentMillis = millis();

    // ----------------------------------------------------------------
    // TASK 2: RUN ENVIRONMENTAL UPDATE & MQTT TELEMETRY (Every 1000ms / 3000ms)
    // ----------------------------------------------------------------
    if (currentMillis - lastEnvironmentTime >= ENVIRONMENT_INTERVAL) {
        lastEnvironmentTime = currentMillis;

        // Update environmental evaluations in background
        ammoniaSubsystem.update();

        // Package and ship system payloads via MQTT
        String ammoniaPayload = String(ammoniaSubsystem.getRawAmmonia());
        mqttManager.publish(MQTT_TOPIC_AMMONIA, ammoniaPayload.c_str());

        String luxPayload = String(growLight.getLux(), 1); 
        mqttManager.publish(MQTT_TOPIC_LIGHT_LUX, luxPayload.c_str());

        String lightstatusPayload = growLight.getLightStatus();
        mqttManager.publish(MQTT_TOPIC_LIGHT_STATUS, lightstatusPayload.c_str());

        mqttManager.publish(MQTT_TOPIC_STATUS, "ONLINE");

        // ====================================================================
        // 📌 PURE RAW LOG VERTICAL REFRESH ENGINE (VS Code Fix)
        // ====================================================================
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

        // --- SHELTER MONITORING STATUS DISPLAY ---
        Serial.print("Occupied Count   : "); Serial.printf("%d / %d Active\n", shelterSubsystem.getLastTriggeredCount() == -1 ? 0 : shelterSubsystem.getLastTriggeredCount(), NUM_SENSORS);
        Serial.print("Timer Countdown  : "); Serial.println(shelterSubsystem.isTrackingActive() ? "[COUNTING DOWN]" : "[IDLE / NO LOCK]");

        Serial.println("-------------------------------------------------");

        // --- NETWORK STATUS ---
        Serial.print("MQTT Connection  : "); 
        if (mqttManager.isConnected()) {
            Serial.println("CONNECTED");
        } else {
            Serial.println("DISCONNECTED");
        }
        Serial.println("=================================================");
    }
}