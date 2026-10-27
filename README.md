# IoT Smart Water Tank Level Monitoring & Overflow Prevention System

An IoT-based embedded system that continuously monitors water levels inside a reservoir, automatically prevents tank overflow by tripping a relay-controlled pump, and streams live telemetry to a cloud dashboard (Blynk IoT). Designed and validated via virtual simulation using the ESP32 microcontroller in Wokwi.

---

## Features
- **Real-Time Level Sensing:** Converts ultrasonic echo timings into tank depth and fill percentages ($0–100\%$).
- **Automated Overflow Prevention:** Shuts off the pump instantly when the water reaches $\ge 90\%$, accompanied by an audio buzzer alarm.
- **Auto-Refill Trigger:** Engages the pump when water levels fall below $\le 20\%$.
- **Two-Way IoT Cloud Control:** Sends continuous data streams (Level %, Distance, Pump Status) to Blynk IoT and accepts remote override commands via a cloud switch.
- **Local Dashboard:** Live updates rendered on a 0.96" SSD1306 I2C OLED display.

---

## System Architecture & Pin Mapping

| ESP32 Pin | Peripheral | Signal / Function |
| :--- | :--- | :--- |
| **GPIO 5** | Ultrasonic Sensor (HC-SR04) | `TRIG` |
| **GPIO 18** | Ultrasonic Sensor (HC-SR04) | `ECHO` |
| **GPIO 23** | Relay Module | `IN` (Active-LOW Pump Control) |
| **GPIO 19** | Active Buzzer | Alarm `+` |
| **GPIO 21** | 0.96" OLED (SSD1306) | `SDA` |
| **GPIO 22** | 0.96" OLED (SSD1306) | `SCL` |
| **3V3 / 5V** | Sensors & Modules | `VCC` |
| **GND** | All Peripherals | Common `GND` |

---

## Blynk IoT Datastream Mapping

| Virtual Pin | Datastream Name | Data Type | Range / Format | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **V0** | Water Level | Integer | 0 – 100 % | Tank Gauge Display |
| **V1** | Distance | Double | 0 – 200 cm | Raw Sensor Measurement |
| **V2** | Pump Switch | Integer | 0 (OFF) / 1 (ON) | Remote Actuator Control |
| **V3** | System Status | String | Text | Diagnostic / Alarm State |

---

## How to Run the Simulation

1. **Configure Blynk Cloud:**
   - Create an ESP32 template on [blynk.cloud](https://blynk.cloud/) with the Datastreams listed above.
   - Obtain your `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME`, and `BLYNK_AUTH_TOKEN`.
2. **Setup Code:**
   - Insert your Blynk credentials into the top of `src/sketch.ino`.
3. **Run on Wokwi:**
   - Open [Wokwi ESP32 Simulator](https://wokwi.com/).
   - Copy `sketch.ino`, `diagram.json`, and `libraries.txt` into the respective tabs.
   - Start the simulation by pressing **Play**.
   - Interact with the slider on the HC-SR04 sensor to test the automatic shutoff and cloud synchronization.
