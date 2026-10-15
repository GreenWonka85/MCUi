/*
  MCUi - Bluetooth Classic connection (Serial Port Profile).
  Only the classic ESP32 has Bluetooth Classic. The app supports it on Android only.
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"

#if defined(ESP32)
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#endif

#if defined(ESP32) && defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED) && defined(SOC_BT_CLASSIC_SUPPORTED) && SOC_BT_CLASSIC_SUPPORTED

#include <BluetoothSerial.h>
#include "esp_gap_bt_api.h"

// Class of Device as MCUi marker: major class "uncategorized", minor class 0x2A.
// The app reads it from paired devices without connecting (Android: 0x1FA8).
#define MCUI_COD_MINOR 0x2A

static BluetoothSerial *g_bt = nullptr;
static bool g_hadClient = false;

namespace mcui_internal {

bool classicBegin(MCUi *, const char *deviceName) {
  if (!g_bt) g_bt = new BluetoothSerial();
  if (!g_bt->begin(deviceName)) return false;
  esp_bt_cod_t cod = {};
  cod.major = ESP_BT_COD_MAJOR_DEV_UNCATEGORIZED;
  cod.minor = MCUI_COD_MINOR;
  esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);
  return true;
}

void classicPoll(MCUi *bench) {
  if (!g_bt) return;
  bool has = g_bt->hasClient();
  if (has != g_hadClient) {
    g_hadClient = has;
    if (has) bench->_onConnect();
    else bench->_onDisconnect();
  }
  uint8_t buf[64];
  size_t n = 0;
  while (g_bt->available() > 0 && n < sizeof(buf)) {
    int c = g_bt->read();
    if (c < 0) break;
    buf[n++] = (uint8_t)c;
  }
  if (n > 0) bench->_onReceive(buf, n);
}

void classicSend(const char *line, size_t len) {
  if (!g_bt || !g_hadClient) return;
  g_bt->write((const uint8_t *)line, len);
  g_bt->write((uint8_t)'\n');
}

}  // namespace mcui_internal

#else  // no Bluetooth Classic (ESP32-S2, -S3, -C3, -C6 ...)

namespace mcui_internal {
bool classicBegin(MCUi *, const char *) { return false; }
void classicPoll(MCUi *) {}
void classicSend(const char *, size_t) {}
}  // namespace mcui_internal

#endif
