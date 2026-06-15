#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>        
#include <PubSubClient.h> 

// ====================================================================
// 📌 WI-FI & MQTT CONFIGURATIONS (Palitan ang mga detalye rito)
// ====================================================================
const char* WIFI_SSID = "PLDT_Home_61D1B";      
const char* WIFI_PASSWORD = "Pldthome123";   
const char* MQTT_SERVER = "192.168.1.172";            
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ====================================================================
// 📌 PIN CONFIGURATIONS
// ====================================================================
const int MQ137_PIN = 34;       // Ammonia Sensor (Analog Pin)
#define TdsSensorPin 35          // TDS Sensor (Analog Pin)
#define FLOW_PIN 18              // Water Flow Sensor (Digital Interrupt Pin)
const int SDA_PIN = 21;          // BH1750 I2C Data
const int SCL_PIN = 22;          // BH1750 I2C Clock

const int RELAY_PIN = 26;        // Ammonia Relay (Low-Level Trigger)
#define TDS_RELAY_PIN 25         // TDS Relay (Low-Level Trigger)
const int LED_PIN = 23;          // Light Dimming LED (PWM Pin)

// ====================================================================
// 📌 SYSTEM THRESHOLDS & SETTINGS
// ====================================================================
const int AMMONIA_THRESHOLD = 1000; 
const int TDS_THRESHOLD = 40; 
const float DARK_THRESHOLD = 1.5;       
const float MEDIUM_THRESHOLD = 500.0;   
const float BRIGHT_THRESHOLD = 1000.0;  

const int LEDC_CHANNEL = 0;             
const int PWM_FREQ = 5000;              
const int PWM_RES = 8;                  
const int BRIGHTNESS_OFF = 0;           
const int BRIGHTNESS_MEDIUM = 76;       
const int BRIGHTNESS_HIGH = 255;        

#define VREF 3.3            
#define SCOUNT 30

// ====================================================================
// 📌 GLOBAL SYSTEM VARIABLES
// ====================================================================
BH1750 lightMeter;

volatile int pulseCount = 0;
float flowRate = 0.0;
int lastSavedPulses = 0; 

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0.0;
float tdsValue = 0.0;
float temperature = 25.0;    

int currentBrightness = 0;
int targetBrightness = 0;
float currentLux = 0.0; 

int globalRawAmmonia = 0;
float globalAmmoniaVolt = 0.0;

// Non-blocking Timing Execution Registries
unsigned long previousFlowTime = 0;
unsigned long analogSampleTimepoint = 0;
unsigned long readDelayLightTime = 0;
unsigned long dashboardUpdateTime = 0; 

// ====================================================================
// 📌 HARDWARE INTERRUPT SERVICE ROUTINES
// ====================================================================
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ====================================================================
// 📌 MATHEMATICAL FILTER CHANNELS
// ====================================================================
int getMedianNum(int bArray[], int iFilterLen) {
    int bTab[iFilterLen];
    for (int i = 0; i < iFilterLen; i++) {
        bTab[i] = bArray[i];
    }
    int bTemp;
    for (int j = 0; j < iFilterLen - 1; j++) {
        for (int i = 0; i < iFilterLen - j - 1; i++) {
            if (bTab[i] > bTab[i + 1]) {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if (iFilterLen & 1) {
        bTemp = bTab[(iFilterLen - 1) / 2];
    } else {
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    }
    return bTemp;
}

// ====================================================================
// 📌 HELPER FUNCTIONS FOR WI-FI & MQTT
// ====================================================================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("[OK] Wi-Fi Connected!");
  Serial.print("IP address ng ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Trying to connect to MQTT Broker at ");
    
    // Sumubok kumonekta gamit ang Client ID na "Craylife_ESP32"
    if (mqttClient.connect("Craylife_ESP32")) {
      Serial.println("\n[OK] Connected to MQTT Broker!");
    } else {
      Serial.print("Failed. State=");
      Serial.print(mqttClient.state());
      Serial.println(" --- Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ====================================================================
// 📌 INITIALIZATION SETUP
// ====================================================================
void setup() {
  Serial.begin(115200);

  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  pinMode(MQ137_PIN, INPUT);
  pinMode(TdsSensorPin, INPUT);
  pinMode(FLOW_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TDS_RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(TDS_RELAY_PIN, HIGH); 

  ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, BRIGHTNESS_OFF);

  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), countPulse, FALLING);

  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  unsigned long startupTime = millis();
  previousFlowTime = startupTime;
  analogSampleTimepoint = startupTime;
  readDelayLightTime = startupTime;
  dashboardUpdateTime = startupTime;

  Serial.print("\e[2J\e[H");
}

// ====================================================================
// 📌 MAIN EXECUTION MONITOR
// ====================================================================
void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop(); 

  unsigned long currentMillis = millis();

  // ------------------------------------------------------------------
  // 📋 BACKGROUND DATA ACQUISITION TRACKS (Non-Blocking)
  // ------------------------------------------------------------------
  globalRawAmmonia = analogRead(MQ137_PIN);
  globalAmmoniaVolt = (globalRawAmmonia / 4095.0) * 3.3;

  if (currentMillis - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = currentMillis;
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex >= SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  if (currentMillis - previousFlowTime >= 1000) {
    previousFlowTime = currentMillis;
    flowRate = pulseCount / 7.5;
    lastSavedPulses = pulseCount; 
    pulseCount = 0; 
  }

  if (currentMillis - readDelayLightTime >= 100) {
    readDelayLightTime = currentMillis;
    currentLux = lightMeter.readLightLevel();

    if (currentLux >= BRIGHT_THRESHOLD) {
      targetBrightness = BRIGHTNESS_OFF;
    } 
    else if (currentLux <= DARK_THRESHOLD) {
      targetBrightness = BRIGHTNESS_HIGH;
    } 
    else {
      targetBrightness = BRIGHTNESS_MEDIUM;
    }

    if (currentBrightness < targetBrightness) {
      currentBrightness++;
      ledcWrite(LEDC_CHANNEL, currentBrightness);
    }
    else if (currentBrightness > targetBrightness) {
      currentBrightness--;
      ledcWrite(LEDC_CHANNEL, currentBrightness);
    }
  }

  // ====================================================================
  // 🖨️ TRACK 5: CLEAN CONSOLIDATED DASHBOARD PRINT ENGINE (Every 3000ms)
  // ====================================================================
  if (currentMillis - dashboardUpdateTime >= 3000) { 
    dashboardUpdateTime = currentMillis;

    // --- Math Pipeline Executions for TDS ---
    for (int i = 0; i < SCOUNT; i++) {
        analogBufferTemp[i] = analogBuffer[i];
    }
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 4095.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                - 255.86 * compensationVoltage * compensationVoltage
                + 857.39 * compensationVoltage) * 0.5;

    // --- Actuator Logic States Determination ---
    bool ammoniaRelayActive = (globalRawAmmonia > AMMONIA_THRESHOLD);
    bool tdsRelayActive = (tdsValue >= TDS_THRESHOLD);

    if (ammoniaRelayActive) digitalWrite(RELAY_PIN, LOW); 
    else digitalWrite(RELAY_PIN, HIGH);

    if (tdsRelayActive) digitalWrite(TDS_RELAY_PIN, LOW); 
    else digitalWrite(TDS_RELAY_PIN, HIGH);

    // --- ANSI Escaped Terminal Dashboard Layout Frame ---
    Serial.print("\e[H"); 
    Serial.println("=================================================================");
    Serial.println("                    CRAYLIFE INTEGRATED SYSTEM                  ");
    Serial.println("=================================================================");
    
    // Module Section 1: Ammonia Monitoring System
    Serial.print("[AMMONIA SYSTEM]  ADC Raw: ");
    Serial.print(globalRawAmmonia);
    Serial.print("   \tVoltage: ");
    Serial.print(globalAmmoniaVolt, 2);
    Serial.print("V\tStatus: ");
    if (ammoniaRelayActive) Serial.println("🚨 CRITICAL! UV ON ");
    else Serial.println("✅ SAFE. UV OFF   ");

    // Module Section 2: Total Dissolved Solids Quality System
    Serial.print("[TDS WATER MGMT]  Density: ");
    Serial.print(tdsValue, 0);
    Serial.print(" ppm\tVoltage: ");
    Serial.print(averageVoltage, 2);
    Serial.print("V\tStatus: ");
    if (tdsRelayActive) Serial.println("🚨 HIGH SOLID! PUMP ON");
    else Serial.println("✅ NORMAL. PUMP OFF  ");

    // Module Section 3: Water Flow Verification Pipeline
    Serial.print("[FLOW MONITOR  ]  Pulses: ");
    Serial.print(lastSavedPulses);
    Serial.print("    \tVelocity: ");
    Serial.print(flowRate, 1);
    Serial.print("L/min\tStatus: ");
    if (lastSavedPulses > 0) Serial.println("✅ WATER FLOWING      ");
    else Serial.println("❌ STATIC RISK NO FLOW");

    // Module Section 4: Photonic Lighting Tracker Calibration 
    Serial.print("[LIGHT CONTROLLER] Intensity: ");
    Serial.print(currentLux, 1);
    Serial.print(" lx\tPWM Output: ");
    Serial.print(currentBrightness);
    Serial.print("/255\tProfile: ");
    if (currentLux >= BRIGHT_THRESHOLD) Serial.println("☀️ BRIGHT (OFF)    ");
    else if (currentLux <= DARK_THRESHOLD) Serial.println("🌙 DARK (HIGH)     ");
    else Serial.println("⛅ MEDIUM (30%)    ");

    Serial.println("=================================================================");

    // ====================================================================
    // 🚀 MQTT TRANSMISSION BLOCK (Ibabato sa Raspi kada 3000ms)
    // ====================================================================
    // Gagawa tayo ng simpleng string payload para madaling basahin ng Python script sa Raspi
    String payload = "{\"ammonia\":" + String(globalRawAmmonia) + 
                     ",\"tds\":" + String(tdsValue, 0) + 
                     ",\"flow\":" + String(flowRate, 1) + 
                     ",\"lux\":" + String(currentLux, 1) + "}";
    
    // I-publish ang payload sa topic na "craylife/sensors"
    mqttClient.publish("craylife/sensors", payload.c_str());
  }
}


// #include <WiFi.h>
// #include <PubSubClient.h>

// const char* ssid = "PLDTHOMEFIBRREbu7";
// const char* password = "PLDTWIFIeRsW4";

// const char* mqtt_server = "192.168.1.8"; // Pi IP

// WiFiClient espClient;
// PubSubClient client(espClient);

// void connectWiFi() {
//     WiFi.begin(ssid, password);

//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }

//     Serial.println("\nWiFi Connected");
// }

// void connectMQTT() {
//     while (!client.connected()) {
//         Serial.println("Connecting MQTT...");

//         if (client.connect("ESP32_Client")) {
//             Serial.println("MQTT Connected");
//         } else {
//             delay(2000);
//         }
//     }
// }

// void setup() {
//     Serial.begin(115200);

//     connectWiFi();

//     client.setServer(mqtt_server, 1883);

//     connectMQTT();
// }

// void loop() {

//     client.publish(
//         "sensor/test",
//         "Hello from ESP32"
//     );

//     Serial.println("Message Sent");

//     delay(5000);
// }