// ESP8266 Alexa to IR Controller (Sinric Pro cloud skill)
// Decoupled architecture: Sinric Pro callbacks set flags that loop() consumes,
// so the blocking IR macros never run inside a network callback.
//
// Devices are registered in the Sinric Pro portal as type "Switch", which Alexa
// presents as a switch rather than a light. This matters: light-typed devices get
// caught by "Alexa, turn off all the lights", which must never fire a 1500W heater.

extern "C" {
    #include "user_interface.h"
}

#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "credentials.h"  // WIFI_SSID, WIFI_PASS, SINRIC_APP_KEY, SINRIC_APP_SECRET, HEATER*_ID

#define SERIAL_BAUDRATE                 115200
#define LED                             4

// IR Command Hex Codes
unsigned int heat_on_off = 0xFFA25D;
unsigned int _500_watts  = 0xFFA857;
unsigned int _1000_watts = 0xFF18E7;
unsigned int _1500_watts = 0xFF4AB5;
unsigned int timer       = 0xFFE817;

const uint16_t kIrLed = 4;  // GPIO pin used to send the IR message (D2 on NodeMCU).
IRsend irsend(kIrLed);

// State Machine Flags for Loop Execution
volatile bool triggerHeater1On  = false;
volatile bool triggerHeater1Off = false;
volatile bool triggerHeater2On  = false;
volatile bool triggerHeater2Off = false;
volatile bool triggerHeater3On  = false;
volatile bool triggerHeater3Off = false;

// -----------------------------------------------------------------------------
// Wifi Setup
// -----------------------------------------------------------------------------
void wifiSetup() {
    WiFi.mode(WIFI_STA);

    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

// -----------------------------------------------------------------------------
// Sinric Pro power callbacks
// Return true to acknowledge the command back to Alexa. Only set flags here --
// the IR macros block for seconds and must not run inside the websocket handler.
// -----------------------------------------------------------------------------
bool onPowerStateHeater1(const String &deviceId, bool &state) {
    Serial.printf("[SINRIC] Heater 1 (%s) -> %s\n", deviceId.c_str(), state ? "ON" : "OFF");
    if (state) triggerHeater1On = true;
    else       triggerHeater1Off = true;
    return true;
}

bool onPowerStateHeater2(const String &deviceId, bool &state) {
    Serial.printf("[SINRIC] Heater 2 (%s) -> %s\n", deviceId.c_str(), state ? "ON" : "OFF");
    if (state) triggerHeater2On = true;
    else       triggerHeater2Off = true;
    return true;
}

bool onPowerStateHeater3(const String &deviceId, bool &state) {
    Serial.printf("[SINRIC] Heater 3 (%s) -> %s\n", deviceId.c_str(), state ? "ON" : "OFF");
    if (state) triggerHeater3On = true;
    else       triggerHeater3Off = true;
    return true;
}

void sinricSetup() {
    SinricProSwitch &heater1 = SinricPro[HEATER1_ID];
    SinricProSwitch &heater2 = SinricPro[HEATER2_ID];
    SinricProSwitch &heater3 = SinricPro[HEATER3_ID];

    heater1.onPowerState(onPowerStateHeater1);
    heater2.onPowerState(onPowerStateHeater2);
    heater3.onPowerState(onPowerStateHeater3);

    SinricPro.onConnected([]() { Serial.println("[SINRIC] Connected to Sinric Pro"); });
    SinricPro.onDisconnected([]() { Serial.println("[SINRIC] Disconnected from Sinric Pro"); });

    SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
}

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("\n\n[BOOT] Initializing system...");

    // Initialize the IR transmitter pin
    irsend.begin();

    wifiSetup();
    sinricSetup();
}

void loop() {
    // Service the Sinric Pro websocket connection
    SinricPro.handle();

    // -------------------------------------------------------------------------
    // Heater Device 1 Sequences
    // -------------------------------------------------------------------------
    if (triggerHeater1On) {
        triggerHeater1On = false; // Reset flag immediately
        Serial.println("Executing: Heater Device 1 ON Sequence");

        Serial.println("Heater init");
        irsend.sendNEC(heat_on_off);
        delay(1000);

        Serial.println("1000 watts");
        irsend.sendNEC(_1000_watts);
        delay(1000);

        Serial.println("Timer On with 1 hr");
        irsend.sendNEC(timer);
        delay(1000);

        irsend.sendNEC(timer);
        Serial.println("Sequence Completed.");
    }

    if (triggerHeater1Off) {
        triggerHeater1Off = false;
        Serial.println("Executing: Heater Device 1 OFF");
        irsend.sendNEC(heat_on_off);
    }

    // -------------------------------------------------------------------------
    // Heater Device 2 Sequences
    // -------------------------------------------------------------------------
    if (triggerHeater2On) {
        triggerHeater2On = false;
        Serial.println("Executing: Heater Device 2 ON (500 watts)");
        irsend.sendNEC(_500_watts);
    }

    if (triggerHeater2Off) {
        triggerHeater2Off = false;
        Serial.println("Executing: Heater Device 2 OFF (1000 watts)");
        irsend.sendNEC(_1000_watts);
    }

    // -------------------------------------------------------------------------
    // Heater Device 3 Sequences
    // -------------------------------------------------------------------------
    if (triggerHeater3On) {
        triggerHeater3On = false;
        Serial.println("Executing: Heater Device 3 ON (1500 watts)");
        irsend.sendNEC(_1500_watts);
    }

    if (triggerHeater3Off) {
        triggerHeater3Off = false;
        Serial.println("Executing: Heater Device 3 OFF Sequence");
        irsend.sendNEC(timer);
        delay(1000);
        irsend.sendNEC(timer);
        Serial.println("Sequence Completed.");
    }

    // -------------------------------------------------------------------------
    // System Diagnostics
    // -------------------------------------------------------------------------
    static unsigned long lastDiagnostics = millis();
    if (millis() - lastDiagnostics > 5000) {
        lastDiagnostics = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }
}
