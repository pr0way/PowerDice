#include "battery.h"
#include "config.h"

namespace Battery {

static uint8_t _pin = 0;

void begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, INPUT);
}

float readVoltage() {
    uint32_t totalMv = 0;
    for (int i = 0; i < BATTERY_SAMPLE_COUNT; ++i) {
        totalMv += analogReadMilliVolts(_pin);
    }
    float avgMv = static_cast<float>(totalMv) / BATTERY_SAMPLE_COUNT;
    return (avgMv * BATTERY_DIVIDER_RATIO) / 1000.0f;
}

} // namespace Battery
