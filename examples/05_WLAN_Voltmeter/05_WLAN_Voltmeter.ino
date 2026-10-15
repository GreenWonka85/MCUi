/*
  MCU Instruments example 05: Voltmeter over Wi-Fi (WLAN)

  Two ways, chosen below with USE_OWN_NETWORK:

  1. Own network (default): the ESP32 opens the Wi-Fi network
     "MCU WLAN Voltmeter". Connect the phone to it (password below), then
     open the app and tap "ESP32 network (192.168.4.1)" under Wi-Fi.
     No router needed. Android may report "no internet"; choose to stay
     connected.

  2. Existing network: the ESP32 joins your Wi-Fi. Phone and ESP32 must be
     in the same network. The app finds the device automatically, or enter
     the IP address shown in the Serial Monitor.

  Works on every ESP32 (including the ESP32-S2 without Bluetooth) and on
  ESP8266 boards (NodeMCU, Wemos D1 mini). Only 2.4 GHz networks.
*/
#include <MCUi.h>

#define USE_OWN_NETWORK 1

// Own network: password with at least 8 characters
const char *AP_PASSWORD = "mcui1234";

// Existing network
const char *WIFI_SSID = "MyNetwork";
const char *WIFI_PASSWORD = "MyPassword";

#if defined(ESP8266)
const int ADC_PIN = A0;  // NodeMCU / Wemos D1 mini: 0 to 3.2 V at A0
#elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
const int ADC_PIN = 4;
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
const int ADC_PIN = 3;
#else
const int ADC_PIN = 34;
#endif

// Voltage divider, see example 01. Without a divider set R1 = 0.
const float R1 = 100000.0;
const float R2 = 10000.0;
const float DIVIDER = (R1 + R2) / R2;

MCUi mcui;

void setup() {
  Serial.begin(115200);

  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.setDecimals("V", 2);

  mcui.setDebug(Serial);  // shows network and IP address in the Serial Monitor
  mcui.setFirmware("1.0");
#if USE_OWN_NETWORK
  mcui.beginWiFiAP("MCU WLAN Voltmeter", AP_PASSWORD);
#else
  mcui.beginWiFi("MCU WLAN Voltmeter", WIFI_SSID, WIFI_PASSWORD);
#endif
}

void loop() {
#if defined(ESP8266)
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(ADC_PIN);
  mcui.set("V", (sum / 16.0) / 1023.0 * 3.2 * DIVIDER);
#else
  if (digitalPinToAnalogChannel(ADC_PIN) >= 0) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(ADC_PIN);
    mcui.set("V", (sum / 16.0) / 1000.0 * DIVIDER);
  }
#endif
  mcui.update();
  delay(10);
}
