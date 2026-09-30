#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// -----------------------------------------------------------------------------
// Sinric Pro credentials -- from https://portal.sinric.pro
//   App Key / App Secret:  Credentials page (one pair per account)
//   Device IDs:            Devices page, one per device; create each as type
//                          "Switch" so Alexa exposes it as a switch and NOT a
//                          light (light-typed devices are hit by "turn off all
//                          the lights", which must never fire a 1500W heater).
// -----------------------------------------------------------------------------
#define SINRIC_APP_KEY    "YOUR_SINRIC_APP_KEY"
#define SINRIC_APP_SECRET "YOUR_SINRIC_APP_SECRET"

#define HEATER1_ID "YOUR_SINRIC_DEVICE_ID_1"
#define HEATER2_ID "YOUR_SINRIC_DEVICE_ID_2"
#define HEATER3_ID "YOUR_SINRIC_DEVICE_ID_3"

#endif // CREDENTIALS_H
