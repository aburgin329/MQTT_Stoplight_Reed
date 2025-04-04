# MQTT Stoplight Garage System

This project demonstrates a garage stoplight system controlled via MQTT. Three ESP8266 devices communicate with an MQTT broker (running on a Mac inside Docker) to control and monitor the system. The devices include:

1. **Stoplight Device**  
   Controls stoplight LEDs (red, yellow, green) and responds to MQTT commands to change light states.

2. **Sensor Device**  
   Uses an HC-SR04 ultrasonic sensor to measure distance and publishes MQTT messages indicating which light (stoplight state) should be activated.

3. **Reed Switch Device**  
   Monitors a reed switch and publishes MQTT messages to enable or disable the system. When disabled, the stoplight and sensor devices ignore commands.

The MQTT broker is provided by Mosquitto running in a Docker container on your Mac.

---

## System Overview

- **Stoplight Device (ESP8266)**  
  - Runs in Access Point (AP) mode (SSID: `ESP8266_Stoplight`) so that other devices, including your Mac, connect to the same network.
  - Subscribes to MQTT topics to receive commands (e.g., `stoplight/green_on`, `stoplight/red_blink_start`, etc.) and controls the LEDs accordingly.
  - Also subscribes to the `system/enable` topic to turn off the stoplights when the system is disabled.

- **Sensor Device (ESP8266)**  
  - Connects to the stoplight device’s AP.
  - Uses an HC-SR04 ultrasonic sensor to measure distance.
  - Publishes MQTT messages to command the stoplight device based on distance thresholds (e.g., publishing to `stoplight/yellow_on`, `stoplight/red_on`, etc.).
  - Also subscribes to `system/enable` to disable publishing when required.

- **Reed Switch Device (ESP8266)**  
  - Connects to the stoplight device’s AP.
  - Reads the state of a reed switch (with or without an internal pull-up resistor).
  - Publishes MQTT messages on the `system/enable` topic:
    - Publishes `"1"` to enable the system.
    - Publishes `"0"` to disable the system.

- **MQTT Broker (Mosquitto in Docker on Mac)**  
  - The broker is run in a Docker container using Mosquitto.
  - A custom configuration file is used to allow remote connections (not “local only”).
  - The container maps the internal port `1883` to the external host port (e.g., `1884`).

---

## Hardware Requirements

- **ESP8266 Modules:** Three units.
- **LEDs:** For stoplight control (red, yellow, green).
- **HC-SR04 Ultrasonic Sensor:** For the Sensor Device.
- **Reed Switch:** For the Reed Switch Device.
- **Mac:** Running Docker Desktop.
- **Router/AP (Optional):** The Stoplight Device acts as an AP; ensure your Mac connects to that network.

---

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) with ESP8266 board support installed.
- [PubSubClient Library](https://github.com/knolleary/pubsubclient) installed via the Arduino Library Manager.
- [Docker Desktop for Mac](https://www.docker.com/products/docker-desktop)
- [Mosquitto Docker Image](https://hub.docker.com/_/eclipse-mosquitto)

---

## MQTT Broker Setup on Mac (Using Docker)

1. **Create a Custom Configuration File**

   Create a file called `mosquitto.conf` in your home directory (or another convenient location) with the following content:

   ```conf
   listener 1883
   allow_anonymous true
