#include "accelerometer.h"
#include "config.h"
#include <Wire.h>

namespace Accelerometer {

// ADXL345 register addresses
namespace Reg {
    constexpr uint8_t POWER_CTL  = 0x2D;
    constexpr uint8_t THRESH_ACT = 0x24;
    constexpr uint8_t ACT_CTL    = 0x27;
    constexpr uint8_t INT_MAP    = 0x2F;
    constexpr uint8_t INT_ENABLE = 0x2E;
    constexpr uint8_t INT_SOURCE = 0x30;
}

static Adafruit_ADXL345_Unified _adxl(12345);

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------
static void writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ADXL345_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------
bool begin() {
    return _adxl.begin();
}

void configure() {
    _adxl.setRange(ADXL345_RANGE_16_G);
}

void getEvent(sensors_event_t& event) {
    _adxl.getEvent(&event);
}

DiceFace getFace() {
    sensors_event_t event;
    _adxl.getEvent(&event);

    const float x = event.acceleration.x;
    const float y = event.acceleration.y;
    const float z = event.acceleration.z;

    if (x >  FACE_DETECT_THRESHOLD) return 3;
    if (x < -FACE_DETECT_THRESHOLD) return 4;
    if (y >  FACE_DETECT_THRESHOLD) return 5;
    if (y < -FACE_DETECT_THRESHOLD) return 2;
    if (z >  FACE_DETECT_THRESHOLD) return 1;
    if (z < -FACE_DETECT_THRESHOLD) return 6;

    return 0; // indeterminate
}

void configureWakeInterrupt() {
    writeReg(Reg::POWER_CTL,  0x08);                  // Measurement mode
    writeReg(Reg::THRESH_ACT, ADXL_ACTIVITY_THRESHOLD);
    writeReg(Reg::ACT_CTL,    0x70);                  // Enable X, Y, Z axes
    writeReg(Reg::INT_MAP,    0x00);                  // Route activity → INT1
    writeReg(Reg::INT_ENABLE, 0x10);                  // Enable Activity interrupt
}

void clearInterrupt() {
    Wire.beginTransmission(ADXL345_I2C_ADDR);
    Wire.write(Reg::INT_SOURCE);
    Wire.endTransmission();
    Wire.requestFrom(static_cast<int>(ADXL345_I2C_ADDR), 1);
    if (Wire.available()) {
        Wire.read();
    }
}

} // namespace Accelerometer
