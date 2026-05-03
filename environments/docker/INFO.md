⚠️ Legacy configuration — retained for reference.

# 🐳 Docker setup
## 🧾 Description:

This is a simplified prototype setup running in Docker on a Synology NAS.

## 🧩 Architecture:

**Dice** (*MQTT Publisher*) → **NanoMQ** (*External MQTT Broker*) ← **Home Assistant** (*MQTT Subscriber*)

## ⚙️ Design notes:
- NanoMQ is used as a standalone MQTT broker
- Authentication is disabled (anonymous access)
- Devices are identified using randomly generated client IDs
- System is intended for local, trusted network environments only