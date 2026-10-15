/*
  MCU Instruments example 10: Voltmeter over Wi-Fi on Arduino boards

  For Arduino boards with a Wi-Fi module:
    - Arduino UNO R4 WiFi                  (library WiFiS3, part of the board package)
    - Arduino Nano 33 IoT                  (library WiFiNINA)
    - Arduino Nano RP2040 Connect          (library WiFiNINA)
    - Arduino MKR WiFi 1010, UNO WiFi Rev2 (library WiFiNINA)
  Install "WiFiNINA" in the Library Manager if your board needs it.

  Two ways, chosen below with USE_OWN_NETWORK:

  1. Own network (default): the board opens the Wi-Fi network
     "MCUi WLAN Voltmeter". Connect the phone to it (password below), then
     open the app. It recognizes the device automatically. No router needed.
     Android may report "no internet"; choose to stay connected.

  2. Existing network: the board joins your Wi-Fi. Phone and board must be
     in the same network. For automatic discovery install the library
     "ArduinoMDNS" and keep USE_MDNS at 1. Without it set USE_MDNS to 0 and
     enter the IP address shown in the Serial Monitor in the app.

  The Wi-Fi library and ArduinoMDNS must be included BEFORE <MCUi.h>.
  Only 2.4 GHz networks. Bluetooth LE and Wi-Fi cannot run at the same time
  on these boards.

  Wiring: voltage divider at A0 as in example 01.
    Input + ---[ R1 100k ]---+---[ R2 10k ]--- GND
                             |
                             A0
  UNO R4 WiFi, UNO WiFi Rev2: max. 5 V at A0. Others: max. 3.3 V at A0.
*/
#define USE_OWN_NETWORK 1
#define USE_MDNS 1  // only for the existing network; 0 if ArduinoMDNS is not installed

#if defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#else
#include <WiFiNINA.h>
#endif
#if USE_MDNS
#include <ArduinoMDNS.h>
#endif
#include <MCUi.h>

// Own network: password with at least 8 characters
const char *AP_PASSWORD = "mcui1234";

// Existing network
const char *WIFI_SSID = "MyNetwork";
const char *WIFI_PASSWORD = "MyPassword";

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
const float VREF = 5.0;
#else
const float VREF = 3.3;
#endif
const float ADC_MAX = 1023.0;  // analogRead() with 10 bits

const float R1 = 100000.0;     // without divider: R1 = 0
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
  if (!mcui.beginWiFiAP("MCUi WLAN Voltmeter", AP_PASSWORD)) {
    Serial.println("Wi-Fi network could not be started.");
  }
#else
  mcui.beginWiFi("MCUi WLAN Voltmeter", WIFI_SSID, WIFI_PASSWORD);
#endif
}

void loop() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(A0);
  mcui.set("V", (sum / 16.0) / ADC_MAX * VREF * DIVIDER);

  mcui.update();
  delay(10);
}
