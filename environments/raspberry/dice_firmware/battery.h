#pragma once
#include <Arduino.h>

namespace Battery {

/**
 * @brief Initialise the battery ADC pin.
 * @param pin GPIO pin connected to the voltage divider mid-point.
 */
void begin(uint8_t pin);

/**
 * @brief Read battery voltage.
 *
 * Takes BATTERY_SAMPLE_COUNT averaged ADC readings and applies the
 * voltage-divider ratio defined in config.h.
 *
 * @return Battery voltage in volts.
 */
float readVoltage();

} // namespace Battery
