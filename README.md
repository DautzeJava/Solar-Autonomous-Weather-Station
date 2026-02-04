# ☀️ SAWS - Solar Autonomous Weather Station

**SAWS** is a prototype for a self-sufficient environmental monitoring system, engineered to balance data accuracy with extreme energy efficiency.

Designed as a final-year STEM project (Math/Science Major), it explores low-power computing, thermodynamics (passive airflow management), and embedded logic optimization.

## 🎯 Key Features

* **Deep Sleep Architecture:** The system utilizes `LowPower` libraries and interrupt-based wake-up routines (via Pin 2) to minimize power consumption when idle.
* **Dual-Power Rail:** I engineered a physical separation of high-load components (LCD Backlight on Pin 10) from critical sensors (Pin 4) to eliminate parasitic drain during sleep mode.
* **Natural Convection Airflow:** The case design leverages the "Chimney Effect" to draw cool air from the bottom and exhaust heat at the top, ensuring readings are independent of the enclosure's internal temperature.
* **Dynamic UI (Language Elevator):** Features a unique "Language Elevator" allowing the user to switch between English, Danish, and French in real-time using the Y-Axis of the joystick.
* **Data Monitoring:** Real-time measurement of Temperature, Humidity, Pressure, Gas Resistance (IAQ), UV Index, and Battery Voltage.

## 🛠️ Hardware Manifest

The system is built around the **ATmega328P** (Arduino Pro Mini 5V/16MHz).

| Component | Function | Notes |
| :--- | :--- | :--- |
| **Microcontroller** | Arduino Pro Mini | 5V version |
| **Sensor A** | BME680 | Temp, Humidity, Pressure, Gas (I2C) |
| **Sensor B** | GUVA-S12SD | Analog UV Sensor |
| **Power Mgmt** | TP4056 + MT3608 | Charging logic & Boost Converter (3.7V to 5V) |
| **Battery** | Li-Po 3.7V | Salvaged from audio equipment (Sustainable design) |
| **Display** | LCD 1602A | 16x2 Characters with I/O optimizations |
| **Input** | Analog Joystick | Menu navigation & Language selection |
| **Solar** | 6V / 3W Panel | Energy harvesting |

## 💻 Software & Libraries

The firmware is written in **C++** using the Arduino framework.
To compile this project, the following libraries are required:

* `Adafruit_BME680` (Sensor driver)
* `Adafruit_Sensor` (Unified sensor abstraction)
* `LowPower` (Power management)
* `LiquidCrystal` (Display driver)
* `Wire` (I2C communication)

## ⚙️ Engineering Highlights

### The "Language Elevator" Logic
Instead of a traditional settings menu, the language selection is mapped to the Joystick Y-Axis using an "elevator" concept:
* **UP:** Danish (DK)
* **CENTER:** English (Default)
* **DOWN:** French (FR)

### Pinout Strategy
* **Pin 4:** Logic Power Rail (Powers Sensors + LCD Logic) -> *Cuts power during sleep.*
* **Pin 10:** Backlight Power Rail (Powers LCD Backlight) -> *Separate control for high current.*
* **Pin 2:** Wake-Up Interrupt Button.
* **A4/A5:** I2C Bus (BME680).

## 🎥 Demonstration

A video demonstration explaining the engineering process is available.
*(Link to be added)*

## 👤 Author

**Félix P.**
*Final Year Student (Advanced Mathematics & Science).*
Project developed for educational purposes and university application portfolio (DTU).

**License:** MIT - Open to use and modification.