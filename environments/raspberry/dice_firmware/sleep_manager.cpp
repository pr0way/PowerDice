#include "sleep_manager.h"
#include "config.h"
#include <Arduino.h>

namespace SleepManager {

[[noreturn]] void enterDeepSleep() {
    const uint64_t gpioMask = 1ULL << PIN_ADXL_INT;
    esp_deep_sleep_enable_gpio_wakeup(gpioMask, ESP_GPIO_WAKEUP_GPIO_HIGH);
    esp_sleep_enable_timer_wakeup(HEARTBEAT_INTERVAL_US);

    Serial.println("[Sleep] Entering deep sleep. Wake sources: GPIO + Timer.");
    Serial.flush();

    esp_deep_sleep_start();
    // Never reached — [[noreturn]] documents this.
}

esp_sleep_wakeup_cause_t getWakeupCause() {
    return esp_sleep_get_wakeup_cause();
}

void printWakeupReason() {
    switch (getWakeupCause()) {
        case ESP_SLEEP_WAKEUP_GPIO:
            Serial.println("[Boot] Woke up by GPIO (shake detected).");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[Boot] Woke up by timer (heartbeat).");
            break;
        default:
            Serial.println("[Boot] Woke up by power-on / reset.");
            break;
    }
}

} // namespace SleepManager
