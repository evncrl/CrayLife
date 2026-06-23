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

// Shelter telemetry topics
#define MQTT_TOPIC_SHELTER_COUNT "craylife/sensor/shelter_count"

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
#define NUM_SENSORS         4
#define SERVO_PIN           32
#define SENSOR_PINS_INIT    {4, 33, 25, 18}

// ====================================================================
// 📌 NTP LIVE TIME FEEDING SCHEDULES (24-Hour Format)
// ====================================================================
#define NTP_SERVER          "pool.ntp.org"
#define GMT_OFFSET_SEC      28800  // UTC+8 Offset for the Philippines (8 hours * 3600 seconds)
#define DAYLIGHT_OFFSET_SEC 0      // No Daylight Savings Time in the Philippines
// --- Breakfast Schedule (8:00 AM) ---
#define BREAKFAST_HOUR      8
#define BREAKFAST_MINUTE    0
// --- Dinner Schedule (8:00 PM) ---
#define DINNER_HOUR         20     // 24-hour equivalent for 8:00 PM
#define DINNER_MINUTE       0

// ====================================================================
// 📌 MQ137 SETTINGS
// ====================================================================
#define AMMONIA_THRESHOLD   1000

// ====================================================================
// 📌 LIGHT THRESHOLDS (LUX)
// ====================================================================
#define DARK_THRESHOLD      5.0        // Triggers full brightness at 5 lux or lower
#define LOW_THRESHOLD       50.0       // Was 100, now lower for more light output
#define MEDIUM_THRESHOLD    300.0      // Was 500, now lower
#define BRIGHT_THRESHOLD    800.0      // Was 1000, now lower

// ====================================================================
// 📌 GROW LIGHT BRIGHTNESS LEVELS
// ====================================================================
#define BRIGHTNESS_OFF      0
#define BRIGHTNESS_LOW      60        // Was 38 (15%), now 60 (24%)
#define BRIGHTNESS_MEDIUM   120       // Was 76 (30%), now 120 (47%)
#define BRIGHTNESS_HIGH     255       // 100% - maximum power

// ====================================================================
// 📌 PWM SETTINGS
// ====================================================================
#define LEDC_CHANNEL        0      // Grow Light (Channel 0)
#define SERVO_LEDC_CHANNEL  2      
#define PWM_FREQ            5000
#define PWM_RES             8

// ====================================================================
// 📌 I2C NOISE FILTERING (Protects BH1750 from servo EMI)
// ====================================================================
#define I2C_CLOCK_STRETCH   200000  // Slow I2C to reduce noise sensitivity

// ====================================================================
// 📌 TIMING SETTINGS
// ====================================================================
#define READ_DELAY_MS         100
#define STABLE_READS_NEEDED   1
#define DIMMING_STEP          10      // Was 50 - faster ramp to full brightness (255/10 = 25ms steps)

// 🆕 SHELTER TIMING & POLARITY SETTINGS
// ====================================================================
#define REQUIRED_TRIGGER_TIME  10000 // 10 seconds in milliseconds
#define SENSOR_POLL_INTERVAL   500   // Run array logic twice every second

#endif // CONFIG_H