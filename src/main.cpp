// ESP8266 Alexa to IR Controller (Wemo Plug Emulation via fauxmoESP v2.4.3)
// Decoupled architecture: Fauxmo callbacks trigger flags in loop to safely handle IR macros.

extern "C" {
    #include "user_interface.h"
}

#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include <fauxmoESP.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "credentials.h"  // Contains WIFI_SSID and WIFI_PASS

#define SERIAL_BAUDRATE                 115200
#define LED                             4

fauxmoESP fauxmo;

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

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("\n\n[BOOT] Initializing system...");

    // Initialize the IR transmitter pin
    irsend.begin();

    wifiSetup();

    // Enable WeMo switch emulation (fauxmoESP 2.4.3 syntax)
    fauxmo.enable(true);

    // Register devices (In fauxmoESP 2.4.3 addDevice returns device_id 0, 1, 2...)
    fauxmo.addDevice("H Device 1D");
    fauxmo.addDevice("H Device 2D");
    fauxmo.addDevice("H Device 3D");


    // Callback for SetBinaryState in fauxmoESP 2.4.3
    // Callback signature: void(unsigned char device_id, const char * device_name, bool state)
    fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
        Serial.printf("[FAUXMO] Callback received for device_id: %d (%s) -> State: %d\n", device_id, device_name, state);
        
        if (strcmp(device_name, "H Device 1D") == 0) {
            if (state) triggerHeater1On = true; 
            else triggerHeater1Off = true;
        }
        else if (strcmp(device_name, "H Device 2D") == 0) {
            if (state) triggerHeater2On = true; 
            else triggerHeater2Off = true;
        }
        else if (strcmp(device_name, "H Device 3D") == 0) {
            if (state) triggerHeater3On = true; 
            else triggerHeater3Off = true;
        }
    });
}

void loop() {
    // Poll UDP packets for Belkin WeMo switch responses
    fauxmo.handle();

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
