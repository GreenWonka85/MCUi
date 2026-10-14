/*
  MCU Instruments example 05: Voltmeter over Wi-Fi (WLAN)

  Same as example 01, but the ESP32 joins your Wi-Fi network instead of
  using Bluetooth. Phone and ESP32 must be in the same network.
  The app finds the device automatically under "Wi-Fi". If not (some
  routers block this), enter the IP address shown in the Serial Monitor.

  Works on every ESP32, including the ESP32-S2 without Bluetooth.
  Only 2.4 GHz networks are supported by the ESP32.
*/
#include <MCUi.h>

// ---- Your Wi-Fi network ----
const char *WIFI_SSID = "MyNetwork";
const char *WIFI_PASSWORD = "MyPassword";

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
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

  mcui.setDebug(Serial);  // shows the IP address in the Serial Monitor
  mcui.setFirmware("1.0");
  mcui.beginWiFi("MCU WLAN Voltmeter", WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  if (digitalPinToAnalogChannel(ADC_PIN) >= 0) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(ADC_PIN);
    mcui.set("V", (sum / 16.0) / 1000.0 * DIVIDER);
  }
  mcui.update();
  delay(10);
}
