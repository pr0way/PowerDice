#pragma once
#include <Arduino.h>

namespace NetManager {

/**
 * @brief Connect to WiFi. Restarts the device on failure.
 */
void connectWifi();

/**
 * @brief Disconnect and power down WiFi (pre-sleep).
 */
void disconnectWifi();

/**
 * @brief Initialise the MQTT client (call after connectWifi).
 */
void mqttSetup();

/**
 * @brief Ensure MQTT is connected; retries up to MQTT_CONNECT_RETRIES.
 * @return true if connected, false if all retries failed.
 */
bool mqttEnsureConnected();

/**
 * @brief Must be called regularly to service MQTT (keep-alive + callbacks).
 */
void mqttLoop();

/**
 * @brief Disconnect MQTT cleanly (pre-sleep).
 */
void mqttDisconnect();

/**
 * @brief Publish a retained message.
 * @param topic MQTT topic string.
 * @param payload Null-terminated payload string.
 * @return true if published successfully.
 */
bool publish(const char* topic, const char* payload);

/**
 * @brief Publish Home Assistant MQTT discovery configs for all entities.
 */
void publishDiscovery();

/**
 * @brief Publish battery voltage as a retained message on TOPIC_BATTERY.
 * @param voltage Voltage in volts.
 */
void publishBattery(float voltage);

/**
 * @brief Publish dice face result as a retained message on TOPIC_ROLL.
 * @param face Face value 1–6.
 */
void publishRoll(uint8_t face);

} // namespace NetManager
