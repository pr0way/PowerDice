#pragma once
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

namespace Accelerometer {

/** Face value 0 = indeterminate (die in motion or tilted). */
using DiceFace = uint8_t;

/**
 * @brief Initialise ADXL345.
 * @return true on success, false if sensor not found.
 */
bool begin();

/**
 * @brief Set range and any required settings after begin().
 */
void configure();

/**
 * @brief Read latest acceleration event.
 * @param[out] event Populated sensor event.
 */
void getEvent(sensors_event_t& event);

/**
 * @brief Determine which face of the die is up.
 *
 * Mapping (adjust to match your physical die orientation):
 *   +X → 3 | −X → 4
 *   +Y → 5 | −Y → 2
 *   +Z → 1 | −Z → 6
 *
 * @return Face value 1–6, or 0 if face cannot be determined.
 */
DiceFace getFace();

/**
 * @brief Configure ADXL345 activity interrupt for GPIO wake-up.
 *
 * Writes directly over I²C so the ESP32 can enter deep sleep while
 * keeping the accelerometer active.
 */
void configureWakeInterrupt();

/**
 * @brief Clear the ADXL345 interrupt latch by reading INT_SOURCE.
 */
void clearInterrupt();

} // namespace Accelerometer
