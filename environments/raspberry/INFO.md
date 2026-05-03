# 🍓 Raspberry Pi setup

## 🧾 Description

This setup runs entirely on Home Assistant OS deployed on a Raspberry Pi 4. It removes external MQTT infrastructure by using the built-in Mosquitto broker add-on.

---

## 🧩 Architecture

**Dice** (*MQTT publisher*) → **Home Assistant** (*MQTT broker via Mosquitto add-on*)

---

## ⚙️ Design notes

- Firmware connects directly to the Home Assistant MQTT broker
- System operates entirely within a single Home Assistant instance
- No external messaging infrastructure is used

---

## 🔐 Authentication model

- MQTT credentials are defined in the Dice firmware (`config.h`)
- A dedicated MQTT user must be created manually either in Home Assistant or in the Mosquitto broker configuration.
- This user account is intended exclusively for the device