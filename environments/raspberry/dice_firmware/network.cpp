#include "network.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

namespace NetManager {

// -------------------------------------------------------------------------
// Module-private state
// -------------------------------------------------------------------------
static WiFiClient   _wifiClient;
static PubSubClient _mqtt(_wifiClient);

// -------------------------------------------------------------------------
// WiFi
// -------------------------------------------------------------------------
void connectWifi() {
    Serial.printf("[WiFi] Connecting to \"%s\"", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (int i = 0; i < WIFI_CONNECT_RETRIES; ++i) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print('.');
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[WiFi] Failed to connect — restarting.");
        ESP.restart();
    }

    Serial.printf("\n[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
}

void disconnectWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Serial.println("[WiFi] Disconnected.");
}

// -------------------------------------------------------------------------
// MQTT
// -------------------------------------------------------------------------
void mqttSetup() {
    _mqtt.setServer(MQTT_SERVER_IP, MQTT_PORT);
    _mqtt.setKeepAlive(MQTT_KEEPALIVE_SEC);
    // Default PubSubClient buffer is 256 bytes — discovery payloads exceed this.
    // 512 bytes comfortably fits all three discovery configs.
    _mqtt.setBufferSize(512);
}

bool mqttEnsureConnected() {
    if (_mqtt.connected()) return true;

    for (int attempt = 1; attempt <= MQTT_CONNECT_RETRIES; ++attempt) {
        Serial.printf("[MQTT] Connecting (attempt %d/%d)...\n", attempt, MQTT_CONNECT_RETRIES);

        bool ok = _mqtt.connect(
            MQTT_CLIENT_ID,
            MQTT_USER,
            MQTT_PASS,
            TOPIC_STATUS, // LWT topic
            0,            // LWT QoS
            true,         // LWT retain
            "offline"     // LWT message
        );

        if (ok) {
            Serial.println("[MQTT] Connected.");
            _mqtt.publish(TOPIC_STATUS, "online", true);
            publishDiscovery();
            return true;
        }

        Serial.printf("[MQTT] Failed, rc=%d. Retrying in %lums.\n",
                      _mqtt.state(), MQTT_RETRY_DELAY_MS);
        delay(MQTT_RETRY_DELAY_MS);
    }

    Serial.println("[MQTT] All retries exhausted.");
    return false;
}

void mqttLoop() {
    _mqtt.loop();
}

void mqttDisconnect() {
    _mqtt.disconnect();
    delay(100);
    Serial.println("[MQTT] Disconnected.");
}

// -------------------------------------------------------------------------
// Publishing helpers
// -------------------------------------------------------------------------
bool publish(const char* topic, const char* payload) {
    bool ok = _mqtt.publish(topic, payload, /*retain=*/true);
    if (!ok) {
        Serial.printf("[MQTT] Publish failed on topic: %s\n", topic);
    }
    return ok;
}

void publishBattery(float voltage) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%.2f", voltage);
    Serial.printf("[Battery] Publishing: %s V (connected=%d)\n", buf, _mqtt.connected());
    bool ok = _mqtt.publish(TOPIC_BATTERY, buf, true);
    Serial.printf("[Battery] Publish result: %d\n", ok);
    _mqtt.loop();
    delay(200);
}

void publishRoll(uint8_t face) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%u", face);
    Serial.printf("[Roll] Publishing face: %s\n", buf);
    _mqtt.publish(TOPIC_ROLL, buf, true);
    _mqtt.loop();
    delay(100);
}

// -------------------------------------------------------------------------
// Home Assistant MQTT Discovery
// -------------------------------------------------------------------------
void publishDiscovery() {
    static const char deviceJson[] =
        "\"device\":{"
          "\"identifiers\":[\"dice_device_001\"],"
          "\"name\":\"Power Dice\"," 
          "\"model\":\"IoT Dice v1\"," 
          "\"manufacturer\":\"DIY\""
        "}";
    static const char avail[] =
        "\"availability_topic\":\"" TOPIC_STATUS "\"," 
        "\"payload_available\":\"online\"," 
        "\"payload_not_available\":\"offline\"";

    char buf[512];

    // --- Roll sensor ---
    snprintf(buf, sizeof(buf),
        "{\"name\":\"Dice Roll\","
        "\"state_topic\":\"" TOPIC_ROLL "\"," 
        "\"unique_id\":\"dice_roll_1\"," 
        "\"icon\":\"mdi:dice-6\"," 
        "\"state_class\":\"measurement\",%s,%s}",
        avail, deviceJson);
    Serial.printf("[MQTT] Roll discovery (%d bytes): %s\n", strlen(buf), buf);
    _mqtt.publish(HA_DISCOVERY_PREFIX "/sensor/dice_roll/config", buf, true);

    // --- Status sensor ---
    snprintf(buf, sizeof(buf),
        "{\"name\":\"Dice Status\"," 
        "\"state_topic\":\"" TOPIC_STATUS "\"," 
        "\"unique_id\":\"dice_status_1\"," 
        "\"icon\":\"mdi:connection\",%s}",
        deviceJson);
    Serial.printf("[MQTT] Status discovery (%d bytes): %s\n", strlen(buf), buf);
    _mqtt.publish(HA_DISCOVERY_PREFIX "/sensor/dice_status/config", buf, true);

    // --- Battery sensor ---
    snprintf(buf, sizeof(buf),
        "{\"name\":\"Dice Battery\"," 
        "\"state_topic\":\"" TOPIC_BATTERY "\"," 
        "\"unique_id\":\"dice_battery_1\"," 
        "\"unit_of_measurement\":\"V\"," 
        "\"device_class\":\"voltage\"," 
        "\"state_class\":\"measurement\"," 
        "\"icon\":\"mdi:battery\",%s,%s}",
        avail, deviceJson);
    Serial.printf("[MQTT] Battery discovery (%d bytes): %s\n", strlen(buf), buf);
    _mqtt.publish(HA_DISCOVERY_PREFIX "/sensor/dice_battery/config", buf, true);

    Serial.println("[MQTT] HA discovery published.");
}


} // namespace NetManager
