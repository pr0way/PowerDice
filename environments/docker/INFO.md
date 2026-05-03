🚨 WARNING: This is legacy code. It works… mostly. Nobody is entirely sure why.

## Hardware Connections

| XIAO Pin | Connection | Target | Description |
|----------|------------|--------|-------------|
| **3V3** | → | ADXL345 (VIN) | Power supply |
| **GND** | → | ADXL345 (GND) | Ground |
| **D0** (GPIO2) | → (via voltage divider) | BAT+/BAT- | Battery voltage monitoring |
| **D1** (GPIO3) | → | ADXL345 (INT1) | Activity interrupt (wake-up) |
| **D4** (GPIO6) | → | ADXL345 (SDA) | I2C data line |
| **D5** (GPIO7) | → | ADXL345 (SCL) | I2C clock line |

**⚠️ Important notes:**
- **Voltage divider:** Use 2× **220kΩ resistors** (original schematic shows 4x 100kΩ, but 220kΩ is recommended)