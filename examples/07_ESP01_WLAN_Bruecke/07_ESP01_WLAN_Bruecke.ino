/*
  MCU Instruments example 07: ESP-01 as Wi-Fi bridge

  Turns an ESP-01 (ESP8266) into a Wi-Fi module for any board: Arduino Uno,
  Nano, Mega, STM32, RP2040, ... Flash this sketch ONCE onto the ESP-01.
  The main board then uses mcui.beginSerial(...) (see example 08).

  The ESP-01 opens its own Wi-Fi network (or joins yours), is found by the
  app under Wi-Fi and passes all data 1:1 between the app and its serial
  port (TX/RX) to the main board.

  Flashing the ESP-01:
    - Tools > Board > "Generic ESP8266 Module" (board package "esp8266 by
      ESP8266 Community", Boards Manager URL
      https://arduino.esp8266.com/stable/package_esp8266com_index.json)
    - A USB adapter for the ESP-01 (with programming switch), or a USB-serial
      adapter at 3.3 V with GPIO0 to GND while powering up.

  Wiring to the main board: see example 08.
  Do not use Serial for anything else in this sketch: everything sent on
  Serial goes to the main board.
*/
#if !defined(ESP8266)
#error "This sketch is for the ESP-01 (ESP8266): Tools > Board > Generic ESP8266 Module"
#endif

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// ---- Settings ----
#define USE_OWN_NETWORK 1                    // 1: own network, 0: join your Wi-Fi
const char *DEVICE_NAME = "MCUi WLAN-Modul";  // name in the app's device list
const char *AP_PASSWORD = "mcui1234";         // own network, at least 8 characters
const char *WIFI_SSID = "MyNetwork";          // existing network
const char *WIFI_PASSWORD = "MyPassword";
const long BAUD = 9600;  // must match the main board (SoftwareSerial: max. 9600 to 19200)
// -------------------

const uint16_t PORT = 4210;

WiFiServer server(PORT);
WiFiClient client;
char deviceId[13];
char host[20];
bool mdnsStarted = false;

void startMdns() {
  if (mdnsStarted || !MDNS.begin(host)) return;
  MDNS.setInstanceName(DEVICE_NAME);
  MDNS.addService("mcui", "tcp", PORT);
  MDNS.addServiceTxt("mcui", "tcp", "id", deviceId);
  MDNS.addServiceTxt("mcui", "tcp", "name", DEVICE_NAME);
  mdnsStarted = true;
}

void setup() {
  Serial.begin(BAUD);

  snprintf(deviceId, sizeof(deviceId), "%06lX%06lX", (unsigned long)(ESP.getChipId() & 0xFFFFFF),
           (unsigned long)(ESP.getFlashChipId() & 0xFFFFFF));
  snprintf(host, sizeof(host), "mcui-%06lx", (unsigned long)(ESP.getChipId() & 0xFFFFFF));

#if USE_OWN_NETWORK
  WiFi.mode(WIFI_AP);
  WiFi.softAP(DEVICE_NAME, AP_PASSWORD);  // reachable at 192.168.4.1
  startMdns();
#else
  WiFi.hostname(host);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  server.begin();
  server.setNoDelay(true);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) startMdns();
  if (mdnsStarted) MDNS.update();

  // New app connection replaces an older one
  WiFiClient incoming = server.accept();
  if (incoming) {
    client.stop();
    client = incoming;
    client.setNoDelay(true);
  }

  uint8_t buf[128];

  // App -> main board
  while (client && client.connected() && client.available() > 0) {
    int n = client.read(buf, sizeof(buf));
    if (n <= 0) break;
    Serial.write(buf, n);
  }

  // Main board -> app
  int avail = Serial.available();
  if (avail > 0) {
    int n = Serial.readBytes(buf, avail < (int)sizeof(buf) ? avail : (int)sizeof(buf));
    if (n > 0 && client && client.connected()) client.write(buf, n);
  }
}
