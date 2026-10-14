/*
  MCUi - Wi-Fi connection: TCP server on port 4210, discovery via mDNS (_mcui._tcp).
  Either in an existing network (station) or as own network (access point, 192.168.4.1).
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"
#include <WiFi.h>
#include <ESPmDNS.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

static WiFiServer *g_server = nullptr;
static WiFiClient *g_client = nullptr;
static bool g_wifiUp = false;
static bool g_mdns = false;
static bool g_apMode = false;
static uint32_t g_lastRssiTxt = 0;
static char g_host[20];

static WiFiClient acceptClient() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
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
  WiFi.setSleep(false);
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
    WiFi.setHostname(g_host);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
  }
  // No power saving: shorter delays for commands from the app
  WiFi.setSleep(false);

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
  if (g_mdns && !g_apMode && up && millis() - g_lastRssiTxt > 5000) {
    g_lastRssiTxt = millis();
    char rssi[8];
    snprintf(rssi, sizeof(rssi), "%d", (int)WiFi.RSSI());
    MDNS.addServiceTxt("mcui", "tcp", "rssi", rssi);
  }

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
