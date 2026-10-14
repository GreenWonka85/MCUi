/*
  MCUi - Bluetooth LE connection (Nordic UART Service).
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"
#include "sdkconfig.h"

#if defined(CONFIG_BT_ENABLED)

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Nordic UART Service (see docs/PROTOCOL.md, section 6.1)
static const char *NUS_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

static MCUi *g_bench = nullptr;
static BLECharacteristic *g_tx = nullptr;

class MCUiServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    if (g_bench) g_bench->_onConnect();
  }
  void onDisconnect(BLEServer *server) override {
    if (g_bench) g_bench->_onDisconnect();
    // Advertise again right away so the app can find the device
    server->startAdvertising();
  }
};

class MCUiRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    // String (core 3.x) or std::string (core 2.x): both offer c_str() and length()
    auto value = characteristic->getValue();
    if (g_bench) g_bench->_onReceive((const uint8_t *)value.c_str(), value.length());
  }
};

namespace mcui_internal {

bool bleBegin(MCUi *bench, const char *deviceName) {
  g_bench = bench;
  BLEDevice::init(deviceName);
  BLEDevice::setMTU(247);

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MCUiServerCallbacks());

  BLEService *service = server->createService(NUS_SERVICE);
  g_tx = service->createCharacteristic(NUS_TX, BLECharacteristic::PROPERTY_NOTIFY);
  g_tx->addDescriptor(new BLE2902());
  BLECharacteristic *rx = service->createCharacteristic(
      NUS_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new MCUiRxCallbacks());
  service->start();

  // Service UUID in the advertisement, name in the scan response
  // (both together do not fit into 31 bytes)
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  BLEAdvertisementData advData;
  advData.setFlags(0x06);
  advData.setCompleteServices(BLEUUID(NUS_SERVICE));
  BLEAdvertisementData scanData;
  scanData.setName(deviceName);
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);
  BLEDevice::startAdvertising();
  return true;
}

void bleSend(const char *line, size_t len, uint16_t chunk) {
  if (!g_tx) return;
  size_t total = len + 1;  // with '\n'
  size_t pos = 0;
  uint8_t buf[244];
  while (pos < total) {
    size_t n = total - pos;
    if (n > chunk) n = chunk;
    if (n > sizeof(buf)) n = sizeof(buf);
    for (size_t i = 0; i < n; i++) {
      size_t p = pos + i;
      buf[i] = p < len ? (uint8_t)line[p] : (uint8_t)'\n';
    }
    g_tx->setValue(buf, n);
    g_tx->notify();
    pos += n;
    delay(3);
  }
}

}  // namespace mcui_internal

#else  // no Bluetooth (e.g. ESP32-S2)

namespace mcui_internal {
bool bleBegin(MCUi *, const char *) { return false; }
void bleSend(const char *, size_t, uint16_t) {}
}  // namespace mcui_internal

#endif
