// #include <Arduino.h>
// #include <Wire.h>
// #include <BH1750.h>

// // =====================
// // MQ137 AIR QUALITY
// // =====================
// const int MQ137_PIN = 34;
// const int RELAY_PIN = 26;
// const int AMMONIA_THRESHOLD = 1000;

// // =====================
// // BH1750 LIGHT SENSOR
// // =====================
// BH1750 lightMeter;

// // Light thresholds
// const float DARK_THRESHOLD = 1.5;
// const float BRIGHT_THRESHOLD = 1000.0;

// // LED PWM
// const int LED_PIN = 23;
// const int LEDC_CHANNEL = 0;
// const int PWM_FREQ = 5000;
// const int PWM_RES = 8;

// // Brightness levels
// const int BRIGHTNESS_OFF = 0;
// const int BRIGHTNESS_MEDIUM = 76;
// const int BRIGHTNESS_HIGH = 255;

// // I2C pins
// const int SDA_PIN = 21;
// const int SCL_PIN = 22;

// // =====================
// // GLOBAL VARIABLES
// // =====================
// int currentBrightness = 0;
// int targetBrightness = 0;

// void setup() {
//   Serial.begin(115200);

//   // =====================
//   // MQ137 SETUP
//   // =====================
//   pinMode(MQ137_PIN, INPUT);
//   pinMode(RELAY_PIN, OUTPUT);
//   digitalWrite(RELAY_PIN, HIGH); // relay OFF (active HIGH mode in your code)

//   // =====================
//   // LED PWM SETUP
//   // =====================
//   ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RES);
//   ledcAttachPin(LED_PIN, LEDC_CHANNEL);
//   ledcWrite(LEDC_CHANNEL, BRIGHTNESS_OFF);

//   // =====================
//   // BH1750 SETUP
//   // =====================
//   Wire.begin(SDA_PIN, SCL_PIN);
//   lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

//   Serial.println("=================================");
//   Serial.println("   COMBINED ESP32 SYSTEM ACTIVE  ");
//   Serial.println("   MQ137 + BH1750 + LED + RELAY  ");
//   Serial.println("=================================");
// }

// void loop() {

//   // =====================
//   // MQ137 SENSOR READ
//   // =====================
//   int rawAmmonia = analogRead(MQ137_PIN);
//   float voltage = (rawAmmonia / 4095.0) * 3.3;

//   Serial.print("[MQ137] ADC: ");
//   Serial.print(rawAmmonia);
//   Serial.print(" | Voltage: ");
//   Serial.print(voltage);

//   if (rawAmmonia > AMMONIA_THRESHOLD) {
//     digitalWrite(RELAY_PIN, HIGH);
//     Serial.println(" -> HIGH AMMONIA! RELAY ON");
//   } else {
//     digitalWrite(RELAY_PIN, LOW);
//     Serial.println(" -> Safe Air. RELAY OFF");
//   }

//   // =====================
//   // BH1750 LIGHT READ
//   // =====================
//   float lux = lightMeter.readLightLevel();

//   if (lux >= BRIGHT_THRESHOLD) {
//     targetBrightness = BRIGHTNESS_OFF;
//     Serial.print("[LIGHT] BRIGHT ");
//     Serial.print(lux);
//     Serial.println(" lx -> LED OFF");
//   }
//   else if (lux <= DARK_THRESHOLD) {
//     targetBrightness = BRIGHTNESS_HIGH;
//     Serial.print("[LIGHT] DARK ");
//     Serial.print(lux);
//     Serial.println(" lx -> LED HIGH");
//   }
//   else {
//     targetBrightness = BRIGHTNESS_MEDIUM;
//     Serial.print("[LIGHT] MEDIUM ");
//     Serial.print(lux);
//     Serial.println(" lx -> LED MEDIUM");
//   }

//   // =====================
//   // SMOOTH LED CONTROL
//   // =====================
//   if (currentBrightness < targetBrightness) {
//     currentBrightness++;
//   } 
//   else if (currentBrightness > targetBrightness) {
//     currentBrightness--;
//   }

//   ledcWrite(LEDC_CHANNEL, currentBrightness);

//   Serial.println("---------------------------------");

//   delay(100);
// }

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

// =====================
// WI-FI & MQTT CONFIG
// =====================
const char* ssid     = "PLDT_Home_61D1B";      // <-- Change to your Wi-Fi Name
const char* password = "Pldthome123";  // <-- Change to your Wi-Fi Password
const char* mqtt_server = "192.168.1.173"; // <-- Change to your MQTT Broker IP/Host
const int mqtt_port = 1883;

// MQTT Topics
const char* topic_ammonia = "esp32/sensor/ammonia";
const char* topic_voltage = "esp32/sensor/voltage";
const char* topic_lux     = "esp32/sensor/lux";

WiFiClient espClient;
PubSubClient client(espClient);

// Timing variable for publishing (avoids flooding the broker)
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 2000; // Publish every 2000ms (2 seconds)

// =====================
// MQ137 AIR QUALITY
// =====================
const int MQ137_PIN = 34;
const int RELAY_PIN = 26;
const int AMMONIA_THRESHOLD = 1000;

// =====================
// BH1750 LIGHT SENSOR
// =====================
BH1750 lightMeter;

// Light thresholds
const float DARK_THRESHOLD = 1.5;
const float BRIGHT_THRESHOLD = 1000.0;

// LED PWM
const int LED_PIN = 23;
const int LEDC_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;

// Brightness levels
const int BRIGHTNESS_OFF = 0;
const int BRIGHTNESS_MEDIUM = 76;
const int BRIGHTNESS_HIGH = 255;

// I2C pins
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// =====================
// GLOBAL VARIABLES
// =====================
int currentBrightness = 0;
int targetBrightness = 0;

// =====================
// HELPER FUNCTIONS
// =====================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(500);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // =====================
  // WI-FI & MQTT SETUP
  // =====================
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // =====================
  // MQ137 SETUP
  // =====================
  pinMode(MQ137_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relay OFF (active HIGH mode in your code)

  // =====================
  // LED PWM SETUP
  // =====================
  ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, BRIGHTNESS_OFF);

  // =====================
  // BH1750 SETUP
  // =====================
  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  Serial.println("=================================");
  Serial.println("   COMBINED ESP32 SYSTEM ACTIVE  ");
  Serial.println("   MQ137 + BH1750 + LED + RELAY  ");
  Serial.println("=================================");
}