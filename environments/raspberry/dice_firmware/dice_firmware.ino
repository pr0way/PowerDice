/**
 * @file dice_firmware.ino
 * @brief IoT Dice — Seeed Xiao ESP32-C3 + ADXL345 + MQTT / Home Assistant
 *
 * Boot modes
 * ----------
 * HEARTBEAT  Woke by weekly timer → send battery report → deep sleep.
 * NORMAL     Woke by shake (or first boot) → enter active session loop.
 *
 * Active session
 * --------------
 * - Reads ADXL345 for stable die face.
 * - Publishes face result over MQTT (retained).
 * - After INACTIVITY_TIMEOUT_SEC seconds of no motion, sleeps.
 *
 * Deep sleep wake sources
 * -----------------------
 * 1. GPIO3 (ADXL345 INT1) — activity interrupt (shake).
 * 2. Timer — HEARTBEAT_INTERVAL_US (default 7 days).
 */

#include <Wire.h>
#include "config.h"
#include "battery.h"
#include "accelerometer.h"
#include "network.h"
#include "sleep_manager.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static Accelerometer::DiceFace lastPublishedFace = 0;
static unsigned long           lastActivityMs    = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void initHardware() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Battery::begin(PIN_BATTERY);
}

static void initAccelerometer() {
    if (!Accelerometer::begin()) {
        Serial.println("[ACCEL] ERROR: ADXL345 not found. Check wiring.");
        Serial.printf("        SDA=GPIO%d  SCL=GPIO%d\n", PIN_I2C_SDA, PIN_I2C_SCL);
        // Halt — no point continuing without sensor.
        while (true) { delay(1000); }
    }
    Accelerometer::configure();
}

static void prepareAndSleep() {
    Accelerometer::clearInterrupt();
    Accelerometer::configureWakeInterrupt();

    NetManager::mqttDisconnect();
    NetManager::disconnectWifi();

    SleepManager::enterDeepSleep(); // [[noreturn]]
}

// ---------------------------------------------------------------------------
// Heartbeat boot path
// ---------------------------------------------------------------------------
static void runHeartbeat() {
    Serial.println("[Mode] HEARTBEAT");

    initHardware();

    if (!Accelerometer::begin()) {
        Serial.println("[Heartbeat] ACCEL missing — aborting heartbeat.");
        ESP.restart();
    }

    NetManager::connectWifi();
    NetManager::mqttSetup();

    if (NetManager::mqttEnsureConnected()) {
        NetManager::publishBattery(Battery::readVoltage());
    }

    prepareAndSleep();
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(100);

    SleepManager::printWakeupReason();

    // Route to heartbeat mode if woken by timer.
    if (SleepManager::getWakeupCause() == ESP_SLEEP_WAKEUP_TIMER) {
        runHeartbeat();
        // runHeartbeat() calls enterDeepSleep() — execution never returns here.
    }

    // ----- NORMAL (active session) boot -----
    Serial.println("[Mode] NORMAL (active session)");

    initHardware();
    initAccelerometer();

    NetManager::connectWifi();
    NetManager::mqttSetup();
    NetManager::mqttEnsureConnected();

    pinMode(PIN_ADXL_INT, INPUT_PULLDOWN);

    lastActivityMs = millis();

    Serial.printf("[Config] Keep-alive:   %d s\n",   MQTT_KEEPALIVE_SEC);
    Serial.printf("[Config] Inactivity:   %lu s\n",  INACTIVITY_TIMEOUT_SEC);
    Serial.printf("[Config] Heartbeat:    %llu days\n",
                  HEARTBEAT_INTERVAL_US / (1000000ULL * 86400ULL));
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop() {
    // 1. Keep MQTT alive.
    if (!NetManager::mqttEnsureConnected()) {
        // Could not reconnect — reset inactivity timer and try next cycle.
        lastActivityMs = millis();
        delay(50);
        return;
    }
    NetManager::mqttLoop();

    // 2. Read acceleration to detect motion.
    sensors_event_t event;
    Accelerometer::getEvent(event);

    const float totalAccel = sqrtf(
        event.acceleration.x * event.acceleration.x +
        event.acceleration.y * event.acceleration.y +
        event.acceleration.z * event.acceleration.z
    );

    if (totalAccel > MOTION_DETECT_THRESHOLD) {
        lastActivityMs = millis();
    }

    // 3. Determine die face and publish if changed.
    const Accelerometer::DiceFace currentFace = Accelerometer::getFace();

    if (currentFace > 0 && currentFace != lastPublishedFace) {
        // Wait for the die to settle, then confirm the reading.
        const unsigned long stabilizeStart = millis();
        while (millis() - stabilizeStart < STABILIZATION_DELAY_MS) {
            NetManager::mqttLoop();
            delay(10);
        }

        if (Accelerometer::getFace() == currentFace) {
            NetManager::publishRoll(currentFace);
            NetManager::publishBattery(Battery::readVoltage());
            lastPublishedFace = currentFace;
            lastActivityMs    = millis();
        }
    }

    // 4. Check inactivity timeout → sleep.
    const unsigned long inactiveMs = millis() - lastActivityMs;
    if (inactiveMs > INACTIVITY_TIMEOUT_SEC * 1000UL) {
        Serial.printf("[Sleep] Inactive for %lu s. Sleeping.\n",
                      inactiveMs / 1000UL);
        prepareAndSleep(); // [[noreturn]]
    }

    delay(50);
}
