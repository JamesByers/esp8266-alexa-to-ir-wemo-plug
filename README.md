# ESP8266 Alexa to IR Controller (WeMo Plug Emulation)

This project allows an **ESP8266** microcontroller (such as a NodeMCU or Adafruit HUZZAH) to emulate **Belkin WeMo Smart Plugs** for local Alexa voice control. When Alexa sends ON or OFF commands to the simulated WeMo devices, the ESP8266 transmits specific Infrared (IR) signals to control devices that do not native smart home capabilities—specifically demonstrated here with a radiant space heater.

---

## 📌 How It Works

1. **WeMo Emulation (`fauxmoESP` v2.2.1)**
   - Emulates Belkin WeMo Smart Plugs over UPnP/UDP (port 1900).
   - Allows local Alexa discovery without requiring third-party cloud services or custom Alexa skills.
2. **Decoupled Asynchronous State Machine**
   - When an Alexa command is received, the `fauxmoESP` callback sets lightweight boolean state flags instantly and returns.
   - The main `loop()` processes these flags and executes multi-step IR macro sequences (including delays).
   - This prevents network packet loss, connection drops, and Soft Watchdog Timer (WDT) resets on the ESP8266.
3. **IR Signal Transmission (`IRremoteESP8266`)**
   - Uses an IR LED connected to GPIO pin 4 (D2 on NodeMCU) to transmit NEC protocol IR hex codes.

---

## 🎛 Supported Alexa Devices & IR Sequences

The code registers 3 virtual WeMo switches with Alexa:

| Device Name | Alexa Command | Triggered Action & IR Macro Sequence |
| :--- | :--- | :--- |
| **Heater device 1h** | **ON** | Sends `Heat ON/OFF` -> waits 1s -> sends `1000W` -> waits 1s -> sends `Timer` twice (1 hr timer). |
| | **OFF** | Sends `Heat ON/OFF`. |
| **Heater device 2h** | **ON** | Sends `500W` IR code. |
| | **OFF** | Sends `1000W` IR code. |
| **Heater device 3h** | **ON** | Sends `1500W` IR code. |
| | **OFF** | Sends `Timer` IR code twice. |

---

## 📁 Repository Structure

```text
esp8266_alexa_to_ir_wemo_plug/
├── platformio.ini              # PlatformIO project configuration & dependencies
├── README.md                   # Project documentation
├── include/
│   ├── credentials.h           # Active Wi-Fi credentials (git-ignored if needed)
│   └── credentials.sample.h    # Credentials template
└── src/
    └── main.cpp                # Main firmware implementation
```

---

## ⚙️ Hardware Setup & Wiring

- **Microcontroller**: NodeMCU v2 (ESP8266) or equivalent.
- **IR Transmitter**: IR LED module or discrete 940nm IR LED with NPN transistor driver circuit.
- **Pin Mapping**:
  - **GPIO 4 (NodeMCU Pin D2)**: Connected to the IR LED transmitter control line.

---

## 🚀 Getting Started

### 1. Prerequisites
- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/platformio-ide) installed (or PlatformIO Core CLI).

### 2. Configure Wi-Fi Credentials
1. Open `include/credentials.h` (or copy from `include/credentials.sample.h`).
2. Update the Wi-Fi credentials with your network details:
   ```cpp
   #define WIFI_SSID "Your_WiFi_Name"
   #define WIFI_PASS "Your_WiFi_Password"
   ```

### 3. Build & Flash
Connect your ESP8266 via USB and run the following commands in the PlatformIO terminal:

```bash
# Build firmware
pio run

# Upload firmware to board
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

---

## 🔊 Alexa Setup & Device Discovery

1. Ensure your ESP8266 and Echo device are connected to the **same local Wi-Fi network**.
2. Power on the ESP8266.
3. Ask Alexa:
   > *"Alexa, discover my devices"*
   *(Alternatively, use the Alexa app -> Devices -> Add Device -> Switch -> Other -> Discover).*
4. Alexa will discover three new switch devices: **"Heater device 1h"**, **"Heater device 2h"**, and **"Heater device 3h"**.
5. Test commands like:
   - *"Alexa, turn on Heater device 1h"*
   - *"Alexa, turn off Heater device 1h"*

---

## 🛠 Troubleshooting & Library Notes

- **FauxmoESP Version**: This project specifically uses **`vintlabs/FauxmoESP@2.2.1`**. Version 2.x emulates Belkin WeMo plugs, whereas Version 3.x switched to Philips Hue bulb emulation.
- **2.4 GHz Wi-Fi**: ESP8266 only supports 2.4 GHz Wi-Fi networks. Ensure your phone/Echo and ESP8266 are on 2.4 GHz during discovery.
- **Multi-AP Routers**: Some mesh/multi-AP routers block UDP multicast packets between devices. Disable AP Isolation on your router if Alexa fails to discover devices.
