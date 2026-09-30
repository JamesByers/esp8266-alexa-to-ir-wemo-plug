# ESP8266 Alexa to IR Controller (Sinric Pro)

This project allows an **ESP8266** microcontroller (such as a NodeMCU or Adafruit HUZZAH) to be controlled by Alexa voice commands, transmitting Infrared (IR) signals to devices that have no native smart home capabilities -- specifically demonstrated here with a radiant space heater.

Alexa reaches the board through the **Sinric Pro** skill: the ESP8266 holds an outbound websocket to the Sinric Pro cloud, and Alexa commands are delivered over it. Voice control therefore requires an internet connection; the IR transmission itself is local.

---

## 📌 How It Works

1. **Cloud Command Path (`SinricPro` v5.1.0)**
   - Devices are registered in the [Sinric Pro portal](https://portal.sinric.pro) as type **Switch** and reach Alexa via the Sinric Pro skill.
   - The board opens an outbound TLS websocket to Sinric Pro, so no port forwarding or inbound access is needed.
2. **Decoupled Asynchronous State Machine**
   - When an Alexa command arrives, the `onPowerState` callback sets lightweight boolean flags instantly and returns `true` to acknowledge.
   - The main `loop()` processes these flags and executes multi-step IR macro sequences (including delays).
   - This keeps the blocking IR macros out of the websocket handler, preventing connection drops and Soft Watchdog Timer (WDT) resets.
3. **IR Signal Transmission (`IRremoteESP8266`)**
   - Uses an IR LED connected to GPIO pin 4 (D2 on NodeMCU) to transmit NEC protocol IR hex codes.

### Why "Switch" and not a light

Sinric Pro devices must be created as type **Switch**. A light-typed device is included in Alexa's light groups, so *"Alexa, turn off all the lights"* or a bedtime routine would fire the heater. Switch-typed devices are excluded from those group commands.

---

## 🎛 Supported Alexa Devices & IR Sequences

Three switches are registered. **Names are configured in the Sinric Pro portal, not in the firmware** -- renaming a device needs no reflash. The names below are the ones currently in use; the firmware matches on device ID.

| Device Name | Alexa Command | Triggered Action & IR Macro Sequence |
| :--- | :--- | :--- |
| **Heater Device 1** | **ON** | Sends `Heat ON/OFF` -> waits 1s -> sends `1000W` -> waits 1s -> sends `Timer` twice (1 hr timer). |
| | **OFF** | Sends `Heat ON/OFF`. |
| **Heater Device 2** | **ON** | Sends `500W` IR code. |
| | **OFF** | Sends `1000W` IR code. |
| **Heater Device 3** | **ON** | Sends `1500W` IR code. |
| | **OFF** | Sends `Timer` IR code twice. |

---

## 📁 Repository Structure

```text
esp8266_alexa_to_ir_wemo_plug/
├── platformio.ini              # PlatformIO project configuration & dependencies
├── README.md                   # Project documentation
├── include/
│   ├── credentials.h           # Wi-Fi + Sinric Pro credentials (git-ignored)
│   └── credentials.sample.h    # Credentials template
├── lib/
│   └── FauxmoESP_2.4.3_Patched/  # Retained for reference only; NOT built (see below)
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
- A free [Sinric Pro](https://portal.sinric.pro) account.

### 2. Create the Sinric Pro devices
1. Sign in at [portal.sinric.pro](https://portal.sinric.pro).
2. On the **Credentials** page, copy the *App Key* (36 characters) and *App Secret* (**73 characters** -- two dash-joined UUID halves; use the copy button, it is easy to under-select).
3. On the **Devices** page, add three devices of type **Switch** and copy each *Device ID* (24 characters).

### 3. Configure Credentials
1. Copy `include/credentials.sample.h` to `include/credentials.h`.
2. Fill in your Wi-Fi details and the five Sinric Pro values:
   ```cpp
   #define WIFI_SSID "Your_WiFi_Name"
   #define WIFI_PASS "Your_WiFi_Password"

   #define SINRIC_APP_KEY    "..."   // 36 chars
   #define SINRIC_APP_SECRET "..."   // 73 chars
   #define HEATER1_ID "..."          // 24 chars
   #define HEATER2_ID "..."
   #define HEATER3_ID "..."
   ```

### 4. Build & Flash
```bash
# Build firmware
pio run

# Upload firmware to board
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

On boot you should see:
```text
[BOOT] Initializing system...
[WIFI] STATION Mode, SSID: <ssid>, IP address: <ip>
[SINRIC] Connected to Sinric Pro
```

---

## 🔊 Alexa Setup

1. Confirm the board shows as **online** in the Sinric Pro portal (that means the websocket is up).
2. Enable the **Sinric Pro** skill and link your Sinric Pro account. In recent Alexa app versions the Skills menu has moved -- the most reliable route is the skill store on the web (`alexa.amazon.com` or `amazon.com/skills`), or searching "Sinric Pro" in the app's search bar.
3. Alexa runs discovery on linking; otherwise say *"Alexa, discover my devices"*.
4. Test:
   - *"Alexa, turn on Heater Device 1"*
   - *"Alexa, turn off Heater Device 1"*

Expected serial output:
```text
[SINRIC] Heater 1 (<deviceId>) -> ON
Executing: Heater Device 1 ON Sequence
Heater init
1000 watts
Timer On with 1 hr
Sequence Completed.
```

---

## 🛠 Troubleshooting & Notes

- **Credential lengths are validated**: the library aborts with `Invalid App Key/Secret ... (Expected: 36 / 73)`. A truncated App Secret is the most common setup error.
- **2.4 GHz Wi-Fi**: ESP8266 only supports 2.4 GHz networks.
- **Flashing**: `upload_speed = 115200`. Do **not** use the 9600 printed on some board silkscreens -- the ESP bootloader rejects it with `Invalid (unsupported) command 0x8`. `upload_port`/`monitor_port` are pinned because auto-detect otherwise selects `/dev/cu.Bluetooth-Incoming-Port` on macOS.
- **Port contention**: do not run VS Code's PlatformIO upload/monitor task at the same time as a terminal flash. Both grab the serial port and the flash fails with `Invalid head of packet` or `could not open port`.

### Why not local WeMo emulation?

This project originally emulated Belkin WeMo plugs locally via `fauxmoESP`, needing no cloud or skill. That no longer works with current Alexa firmware. Diagnosis showed the board completed the entire WeMo handshake -- M-SEARCH received, UDP responses sent, and **all three `setup.xml` descriptions fetched by the Echo** -- after which Alexa never registered the devices and never requested `eventservice.xml` or any `basicevent` state. This matches the unresolved upstream report [vintlabs/fauxmoESP#288](https://github.com/vintlabs/fauxmoESP/issues/288).

`lib/FauxmoESP_2.4.3_Patched/` is retained for reference and is **not** in `lib_deps`. It contains upstream tag 2.4.3 plus three local fixes:

1. **Stack overrun** -- `handle()` allocated the UDP read buffer as `data[len]` while `_onUDPData()` NUL-terminates at `p[len]`, writing one byte past the end and crashing the board with `Exception (28)` on every M-SEARCH. Changed to `data[len + 1]`.
2. **Dropped probes** -- upstream ignored every incoming M-SEARCH while a response burst was in flight (~4s), so Alexa's re-probes were discarded. Removed that gate; responses remain paced by `UDP_RESPONSES_INTERVAL`.
3. **Unfair advertising** -- upstream started each burst at a random device index, which could starve a device for a whole discovery cycle. Now starts at 0 deterministically.

Hue emulation (`fauxmoESP` 3.x) was rejected deliberately: it registers devices as **lights**, which would let *"turn off all the lights"* control a 1500 W heater.
