/*
  MCUi - Wi-Fi connection: TCP server on port 4210, discovery via mDNS (_mcui._tcp).
  Either in an existing network (station) or as own network (access point, 192.168.4.1).
  Boards: ESP32 (all), ESP8266, Raspberry Pi Pico W / Pico 2 W, and with their Wi-Fi library
  included in the sketch: UNO R4 WiFi (WiFiS3), Nano 33 IoT, Nano RP2040 Connect,
  MKR WiFi 1010, UNO WiFi Rev2 (WiFiNINA). Optional ArduinoMDNS for discovery there.
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"

// Raspberry Pi Pico W / Pico 2 W with the core "Raspberry Pi Pico/RP2040" (Earle Philhower)
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED) && \
    (defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_2W) || defined(PICO_CYW43_SUPPORTED))
#define MCUI_PICO_W 1
#endif

// Arduino boards with Wi-Fi module: UNO R4 WiFi (WiFiS3), Nano 33 IoT, Nano RP2040 Connect,
// MKR WiFi 1010, UNO WiFi Rev2 (WiFiNINA). The sketch includes the Wi-Fi library;
// optionally ArduinoMDNS for automatic discovery in an existing network.
#if defined(ARDUINO_UNOR4_WIFI) && __has_include(<WiFiS3.h>)
#define MCUI_ARDUINO_WIFI 1
#define MCUI_ARDUINO_WIFI_HEADER <WiFiS3.h>
#elif (defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT) || \
       defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2)) && __has_include(<WiFiNINA.h>)
#define MCUI_ARDUINO_WIFI 1
#define MCUI_ARDUINO_WIFI_HEADER <WiFiNINA.h>
#endif

#if defined(ESP32) || defined(ESP8266) || defined(MCUI_PICO_W)

#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <LEAmDNS.h>
#endif

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

static WiFiServer *g_server = nullptr;
static WiFiClient *g_client = nullptr;
static bool g_wifiUp = false;
static bool g_mdns = false;
static bool g_apMode = false;
#if defined(ESP32)
static uint32_t g_lastRssiTxt = 0;
#endif
static char g_host[20];

static WiFiClient acceptClient() {
#if defined(ESP8266) || defined(MCUI_PICO_W) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3)
  return g_server->accept();
#else
  return g_server->available();
#endif
}

namespace mcui_internal {

static void makeHostName(MCUi *bench) {
  // Host name from the device ID, e.g. "mcui-a4cf12"
  const char *id = bench->_deviceId();
  size_t idLen = strlen(id);
  snprintf(g_host, sizeof(g_host), "mcui-%s", idLen >= 6 ? id + idLen - 6 : id);
  for (char *p = g_host; *p; p++) *p = (char)tolower(*p);
}

static void noSleep() {
#if defined(ESP8266)
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
#elif defined(MCUI_PICO_W)
  WiFi.noLowPowerMode();
#else
  WiFi.setSleep(false);
#endif
}

static void startServer() {
  if (!g_server) g_server = new WiFiServer(MCUI_WIFI_PORT);
  if (!g_client) g_client = new WiFiClient();
  g_server->begin();
  g_server->setNoDelay(true);
}

bool wifiBeginAP(MCUi *bench, const char *ssid, const char *password) {
  makeHostName(bench);
  g_apMode = true;
  Print *dbg = bench->_debugOut();
  // WPA2 needs at least 8 characters
  if (password && strlen(password) < 8) {
    if (dbg) dbg->println("MCUi: Wi-Fi password shorter than 8 characters, network stays open");
    password = nullptr;
  }
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ssid, password);
  noSleep();
  startServer();
  if (dbg) {
    if (ok) {
      dbg->printf("MCUi: Wi-Fi network \"%s\" %s, IP %s, port %d\n", ssid, password ? "(WPA2)" : "(open)",
                  WiFi.softAPIP().toString().c_str(), MCUI_WIFI_PORT);
    } else {
      dbg->println("MCUi: could not start the Wi-Fi network");
    }
  }
  return ok;
}

bool wifiBegin(MCUi *bench, const char *deviceName, const char *ssid, const char *password) {
  makeHostName(bench);

  if (ssid) {
#if defined(ESP8266)
    WiFi.hostname(g_host);
#else
    WiFi.setHostname(g_host);
#endif
    WiFi.mode(WIFI_STA);
#if !defined(MCUI_PICO_W)
    WiFi.setAutoReconnect(true);
#endif
    WiFi.begin(ssid, password);
  }
  // No power saving: shorter delays for commands from the app
  noSleep();

  startServer();

  Print *dbg = bench->_debugOut();
  if (dbg) {
    if (ssid) dbg->printf("MCUi: connecting to Wi-Fi \"%s\" as %s ...\n", ssid, g_host);
    else dbg->printf("MCUi: Wi-Fi server started as %s\n", g_host);
  }
  (void)deviceName;
  return true;
}

void wifiPoll(MCUi *bench) {
  if (!g_server) return;
#if defined(ESP8266) || defined(MCUI_PICO_W)
  if (g_mdns) MDNS.update();
#endif
  Print *dbg = bench->_debugOut();

  // Network up or down
  bool up = g_apMode || WiFi.status() == WL_CONNECTED;
  if (up != g_wifiUp) {
    g_wifiUp = up;
    if (up) {
      if (dbg && !g_apMode) {
        dbg->printf("MCUi: Wi-Fi connected, IP %s, port %d\n", WiFi.localIP().toString().c_str(), MCUI_WIFI_PORT);
      }
      if (!g_mdns && MDNS.begin(g_host)) {
        MDNS.setInstanceName(bench->_deviceName());
        MDNS.addService("mcui", "tcp", MCUI_WIFI_PORT);
        MDNS.addServiceTxt("mcui", "tcp", "id", bench->_deviceId());
        MDNS.addServiceTxt("mcui", "tcp", "name", bench->_deviceName());
        g_mdns = true;
      }
    } else if (dbg) {
      dbg->println("MCUi: Wi-Fi lost, reconnecting ...");
    }
  }

  // Signal strength to the router as mDNS TXT record "rssi", so the app can show it
  // in the device list (only in an existing network; in access point mode the phone
  // measures the signal itself)
#if defined(ESP32)
  if (g_mdns && !g_apMode && up && millis() - g_lastRssiTxt > 5000) {
    g_lastRssiTxt = millis();
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)WiFi.RSSI());
    // const char* for all four arguments, otherwise the call is ambiguous
    const char *rssi = buf;
    MDNS.addServiceTxt("mcui", "tcp", "rssi", rssi);
  }
#endif

  // New app connection. It replaces an older one, e.g. after the phone lost Wi-Fi.
  WiFiClient incoming = acceptClient();
  if (incoming) {
    if (bench->isConnected()) {
      g_client->stop();
      bench->_onDisconnect();
    }
    *g_client = incoming;
    g_client->setNoDelay(true);
    bench->_onConnect();
    if (dbg) dbg->printf("MCUi: app connected from %s\n", g_client->remoteIP().toString().c_str());
  }

  // Connection closed by the app or lost
  if (bench->isConnected() && !g_client->connected()) {
    g_client->stop();
    bench->_onDisconnect();
    return;
  }

  // Received data
  if (bench->isConnected()) {
    uint8_t buf[64];
    while (g_client->available() > 0) {
      int n = g_client->read(buf, sizeof(buf));
      if (n <= 0) break;
      bench->_onReceive(buf, (size_t)n);
    }
  }
}

void wifiSend(const char *line, size_t len) {
  if (!g_client || !g_client->connected()) return;
  // Line and '\n' in one packet
  uint8_t buf[520];
  if (len + 1 <= sizeof(buf)) {
    memcpy(buf, line, len);
    buf[len] = '\n';
    g_client->write(buf, len + 1);
  } else {
    g_client->write((const uint8_t *)line, len);
    g_client->write((uint8_t)'\n');
  }
}

}  // namespace mcui_internal

#elif defined(MCUI_ARDUINO_WIFI)

// ---------------------------------------------------------------- WiFiS3 / WiFiNINA

#include MCUI_ARDUINO_WIFI_HEADER

#if __has_include(<ArduinoMDNS.h>)
#include <ArduinoMDNS.h>
#define MCUI_ARDUINO_MDNS 1
static WiFiUDP g_udp;
static MDNS g_mdnsResponder(g_udp);
static char g_mdnsName[48];
static char g_txt[72];
#endif

static WiFiServer *g_server = nullptr;
static WiFiClient g_client;
static bool g_apMode = false;
static bool g_wifiUp = false;
static bool g_serverStarted = false;
static bool g_mdns = false;
static const char *g_ssid = nullptr;
static const char *g_password = nullptr;
static uint32_t g_lastTry = 0;
static char g_host[20];

namespace mcui_internal {

static void makeHostName(MCUi *bench) {
  const char *id = bench->_deviceId();
  size_t idLen = strlen(id);
  snprintf(g_host, sizeof(g_host), "mcui-%s", idLen >= 6 ? id + idLen - 6 : id);
  for (char *p = g_host; *p; p++) *p = (char)tolower(*p);
}

static void printIp(Print *dbg, IPAddress ip) {
  dbg->print(ip[0]);
  dbg->print('.');
  dbg->print(ip[1]);
  dbg->print('.');
  dbg->print(ip[2]);
  dbg->print('.');
  dbg->print(ip[3]);
}

static void startMdns(MCUi *bench) {
#if defined(MCUI_ARDUINO_MDNS)
  if (g_mdns) return;
  if (!g_mdnsResponder.begin(WiFi.localIP(), g_host)) return;
  // Service name "<instance>._mcui"; TXT entries with a length byte in front
  snprintf(g_mdnsName, sizeof(g_mdnsName), "%s._mcui", bench->_deviceName());
  const char *id = bench->_deviceId();
  const char *name = bench->_deviceName();
  size_t idLen = strlen(id) + 3, nameLen = strlen(name) + 5;
  if (nameLen > 40) nameLen = 40;
  size_t pos = 0;
  g_txt[pos++] = (char)idLen;
  pos += snprintf(g_txt + pos, sizeof(g_txt) - pos, "id=%s", id);
  if (pos + 1 + nameLen < sizeof(g_txt)) {
    g_txt[pos++] = (char)nameLen;
    snprintf(g_txt + pos, nameLen + 1, "name=%s", name);
    pos += nameLen;
  }
  g_txt[pos] = 0;
  g_mdnsResponder.addServiceRecord(g_mdnsName, MCUI_WIFI_PORT, MDNSServiceTCP, g_txt);
  g_mdns = true;
#else
  (void)bench;
#endif
}

bool wifiBeginAP(MCUi *bench, const char *ssid, const char *password) {
  makeHostName(bench);
  g_apMode = true;
  Print *dbg = bench->_debugOut();
  if (password && strlen(password) < 8) {
    if (dbg) dbg->println(F("MCUi: Wi-Fi password shorter than 8 characters, network stays open"));
    password = nullptr;
  }
  uint8_t st = password ? WiFi.beginAP(ssid, password) : WiFi.beginAP(ssid);
  bool ok = st == WL_AP_LISTENING;
  if (!g_server) g_server = new WiFiServer(MCUI_WIFI_PORT);
  if (dbg) {
    if (ok) {
      dbg->print(F("MCUi: Wi-Fi network \""));
      dbg->print(ssid);
      dbg->print(F("\", IP "));
      printIp(dbg, WiFi.localIP());
      dbg->println(F(", port 4210"));
    } else {
      dbg->println(F("MCUi: could not start the Wi-Fi network (update the Wi-Fi module firmware?)"));
    }
  }
  return ok;
}

bool wifiBegin(MCUi *bench, const char *deviceName, const char *ssid, const char *password) {
  (void)deviceName;
  makeHostName(bench);
  WiFi.setHostname(g_host);
  g_ssid = ssid;
  g_password = password;
  if (!g_server) g_server = new WiFiServer(MCUI_WIFI_PORT);
  Print *dbg = bench->_debugOut();
  if (ssid) {
    if (dbg) {
      dbg->print(F("MCUi: connecting to Wi-Fi \""));
      dbg->print(ssid);
      dbg->println(F("\" ..."));
    }
    WiFi.begin(ssid, password);  // waits a few seconds
    g_lastTry = millis();
  }
  return true;
}

void wifiPoll(MCUi *bench) {
  if (!g_server) return;
  Print *dbg = bench->_debugOut();

  bool up = g_apMode || WiFi.status() == WL_CONNECTED;
  if (up != g_wifiUp) {
    g_wifiUp = up;
    if (up) {
      if (!g_serverStarted) {
        g_server->begin();
        g_serverStarted = true;
      }
      startMdns(bench);
      if (dbg && !g_apMode) {
        dbg->print(F("MCUi: Wi-Fi connected, IP "));
        printIp(dbg, WiFi.localIP());
        dbg->println(F(", port 4210"));
      }
    } else if (dbg) {
      dbg->println(F("MCUi: Wi-Fi lost, reconnecting ..."));
    }
  }
  // Reconnect in an existing network (WiFi.begin() blocks for a few seconds)
  if (!up && !g_apMode && g_ssid && millis() - g_lastTry > 15000) {
    g_lastTry = millis();
    WiFi.begin(g_ssid, g_password);
  }
#if defined(MCUI_ARDUINO_MDNS)
  if (g_mdns) g_mdnsResponder.run();
#endif
  if (!g_serverStarted) return;

  // Connection lost
  if (bench->isConnected() && !g_client.connected()) {
    g_client.stop();
    bench->_onDisconnect();
  }
  // New app connection (one at a time; the app sends HELLO right away)
  if (!bench->isConnected()) {
    WiFiClient incoming = g_server->available();
    if (incoming) {
      g_client = incoming;
      bench->_onConnect();
    }
  }
  // Received data
  if (bench->isConnected()) {
    uint8_t buf[64];
    while (g_client.available() > 0) {
      int n = g_client.read(buf, sizeof(buf));
      if (n <= 0) break;
      bench->_onReceive(buf, (size_t)n);
    }
  }
}

void wifiSend(const char *line, size_t len) {
  if (!g_client.connected()) return;
  g_client.write((const uint8_t *)line, len);
  g_client.write((uint8_t)'\n');
}

}  // namespace mcui_internal

#else  // no supported Wi-Fi (AVR, STM32, ...): use an ESP-01 bridge with beginSerial()

namespace mcui_internal {
bool wifiBegin(MCUi *, const char *, const char *, const char *) { return false; }
bool wifiBeginAP(MCUi *, const char *, const char *) { return false; }
void wifiPoll(MCUi *) {}
void wifiSend(const char *, size_t) {}
}  // namespace mcui_internal

#endif
