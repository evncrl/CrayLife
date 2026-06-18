#ifndef CONFIG_H
#define CONFIG_H

// ====================================================================
// 📌 WI-FI CONFIGURATIONS
// ====================================================================
#define WIFI_SSID           "PLDT_Home_61D1B"
#define WIFI_PASSWORD       "Pldthome123"

// ====================================================================
// 📌 MQTT BROKER CONFIGURATIONS
// ====================================================================
#define MQTT_SERVER         "192.168.1.173"
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "ESP32_CrayLife_Subsystem"

// --- MQTT Topics ---
#define MQTT_TOPIC_AMMONIA       "craylife/sensor/ammonia"
#define MQTT_TOPIC_STATUS        "craylife/status"
#define MQTT_TOPIC_CONTROL       "craylife/control/uv"

// Optional Grow Light Topics
#define MQTT_TOPIC_LIGHT_LUX     "craylife/sensor/light"
#define MQTT_TOPIC_LIGHT_STATUS  "craylife/growlight/status"

// ====================================================================
// 📌 HARDWARE PINS (ESP32)
// ====================================================================

// MQ137 Ammonia Sensor
#define MQ137_PIN           34
#define RELAY_PIN           26

// BH1750 Light Sensor
#define SDA_PIN             21
#define SCL_PIN             22

// Grow Light MOSFET
#define GROWLIGHT_PIN       23

// 🆕 Shelter Array Pins
#define NUM_SENSORS         5
#define SERVO_PIN           32
#define SENSOR_PINS_INIT    {4, 16, 17, 5, 18}

// ====================================================================
// 📌 MQ137 SETTINGS
// ====================================================================
#define AMMONIA_THRESHOLD   1000

// ====================================================================
// 📌 LIGHT THRESHOLDS (LUX)
// ====================================================================
#define DARK_THRESHOLD      1.5
#define LOW_THRESHOLD       100.0
#define MEDIUM_THRESHOLD    500.0
#define BRIGHT_THRESHOLD    1000.0

// ====================================================================
// 📌 GROW LIGHT BRIGHTNESS LEVELS
// ====================================================================
#define BRIGHTNESS_OFF      0
#define BRIGHTNESS_LOW      38      // 15%
#define BRIGHTNESS_MEDIUM   76      // 30%
#define BRIGHTNESS_HIGH     255     // 100%

// ====================================================================
// 📌 PWM SETTINGS
// ====================================================================
#define LEDC_CHANNEL        0
#define PWM_FREQ            5000
#define PWM_RES             8

// ====================================================================
// 📌 TIMING SETTINGS
// ====================================================================
#define READ_DELAY_MS         100
#define STABLE_READS_NEEDED   1
#define DIMMING_STEP          50

// 🆕 SHELTER TIMING & POLARITY SETTINGS
// ====================================================================
#define REQUIRED_TRIGGER_TIME  10000 // 10 seconds in milliseconds
#define SENSOR_POLL_INTERVAL   500   // Run array logic twice every second

#endif // CONFIG_H