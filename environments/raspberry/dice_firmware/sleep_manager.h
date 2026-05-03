#pragma once
#include <esp_sleep.h>

namespace SleepManager {

/**
 * @brief Configure wake-up sources and enter deep sleep.
 *
 * Enables two wake sources:
 *   1. GPIO — ADXL345 INT1 activity interrupt (shake detection)
 *   2. Timer — periodic heartbeat (HEARTBEAT_INTERVAL_US)
 *
 * Callers must flush Serial and disconnect WiFi/MQTT before calling this.
 */
[[noreturn]] void enterDeepSleep();

/**
 * @brief Return the wakeup cause for the current boot.
 */
esp_sleep_wakeup_cause_t getWakeupCause();

/**
 * @brief Print wakeup reason to Serial for diagnostics.
 */
void printWakeupReason();

} // namespace SleepManager
