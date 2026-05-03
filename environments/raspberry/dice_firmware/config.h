#pragma once

// =============================================================================
// NETWORK CONFIGURATION
// =============================================================================
#define WIFI_SSID        ""           // <--- ENTER YOUR WIFI NAME
#define WIFI_PASSWORD    ""           // <--- ENTER YOUR WIFI PASSWORD
#define MQTT_SERVER_IP   "192.168.1.X"  // <--- ENTER YOUR MQTT BROKER IP
#define MQTT_PORT        1883

// =============================================================================
// MQTT CREDENTIALS & TOPICS
// =============================================================================
#define MQTT_USER        "dice" // <--- ENTER YOUR MQTT USERNAME
#define MQTT_PASS        "" // <--- ENTER YOUR MQTT PASSWORD
#define MQTT_CLIENT_ID   "dice_device_001"
#define MQTT_PREFIX      "dice"

#define TOPIC_ROLL       MQTT_PREFIX "/roll"
#define TOPIC_STATUS     MQTT_PREFIX "/status"
#define TOPIC_BATTERY    MQTT_PREFIX "/battery"

#define HA_DISCOVERY_PREFIX  "homeassistant"

// =============================================================================
// HARDWARE PINS (Seeed Xiao ESP32-C3)
// =============================================================================
#define PIN_BATTERY      2   // GPIO2 — ADC1_CH2 for battery voltage divider
#define PIN_ADXL_INT     3   // GPIO3 — ADXL345 INT1 wake-up pin
#define PIN_I2C_SDA      6   // GPIO6 / D4
#define PIN_I2C_SCL      7   // GPIO7 / D5

#define ADXL345_I2C_ADDR 0x53

// =============================================================================
// BATTERY
// =============================================================================
// Voltage divider: 220kΩ / 220kΩ → ratio = 2.0
#define BATTERY_DIVIDER_RATIO   2.0f
#define BATTERY_SAMPLE_COUNT    16

// =============================================================================
// MOTION THRESHOLDS
// =============================================================================
// Axis threshold (m/s²) for face detection — must exceed gravity on one axis
#define FACE_DETECT_THRESHOLD    8.0f

// Total acceleration (m/s²) above which we consider the die "in motion"
#define MOTION_DETECT_THRESHOLD  15.0f

// ADXL345 activity threshold register value (62.5 mg/LSB)
// 20 → ~1.25 g  — adjust for desired sensitivity
#define ADXL_ACTIVITY_THRESHOLD  20

// =============================================================================
// TIMING
// =============================================================================
// Milliseconds to wait for a stable face reading before publishing
#define STABILIZATION_DELAY_MS   500UL

// Seconds of inactivity before entering deep sleep
#define INACTIVITY_TIMEOUT_SEC   120UL

// Weekly heartbeat timer (microseconds)
#define HEARTBEAT_INTERVAL_US    (7ULL * 24ULL * 60ULL * 60ULL * 1000000ULL)

// =============================================================================
// MQTT
// =============================================================================
#define MQTT_KEEPALIVE_SEC    45
#define MQTT_CONNECT_RETRIES  3
#define MQTT_RETRY_DELAY_MS   5000UL

// =============================================================================
// WIFI
// =============================================================================
#define WIFI_CONNECT_RETRIES  20
#define WIFI_RETRY_DELAY_MS   500UL
