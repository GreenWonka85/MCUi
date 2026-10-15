/*
  MCUi - Bluetooth LE connection (Nordic UART Service).

  ESP32:  built-in BLE library of the ESP32 core.
  Others: official ArduinoBLE library (Nano 33 BLE, Nano 33 IoT, UNO R4 WiFi,
          Nano RP2040 Connect, Portenta, ...). Install "ArduinoBLE" in the
          Library Manager and add #include <ArduinoBLE.h> to the sketch.
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"

#if defined(ESP32)
#include "sdkconfig.h"
#endif

#if defined(ESP32) && defined(CONFIG_BT_ENABLED)

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
  // MCUi marker so the app shows only devices with this library:
  // company ID 0xFFFF (no assigned company), "MCUi", protocol version
  const char marker[] = {(char)0xFF, (char)0xFF, 'M', 'C', 'U', 'i', (char)MCUI_PROTOCOL_VERSION, 0};
  advData.setManufacturerData(marker);
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

void blePoll(MCUi *) {}

}  // namespace mcui_internal

#elif !defined(ESP32) && __has_include(<ArduinoBLE.h>)

// ---------------------------------------------------------------- ArduinoBLE

#include <ArduinoBLE.h>

static MCUi *g_bench = nullptr;
static BLEService g_service("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLECharacteristic g_rx("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite | BLEWriteWithoutResponse, 244);
static BLECharacteristic g_tx("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify, 244);

static void onConnected(BLEDevice) {
  if (g_bench) g_bench->_onConnect();
}

static void onDisconnected(BLEDevice) {
  if (g_bench) g_bench->_onDisconnect();
  BLE.advertise();
}

static void onWritten(BLEDevice, BLECharacteristic characteristic) {
  if (g_bench) g_bench->_onReceive(characteristic.value(), characteristic.valueLength());
}

namespace mcui_internal {

bool bleBegin(MCUi *bench, const char *deviceName) {
  g_bench = bench;
  if (!BLE.begin()) return false;
  BLE.setDeviceName(deviceName);

  g_service.addCharacteristic(g_rx);
  g_service.addCharacteristic(g_tx);
  BLE.addService(g_service);
  g_rx.setEventHandler(BLEWritten, onWritten);
  BLE.setEventHandler(BLEConnected, onConnected);
  BLE.setEventHandler(BLEDisconnected, onDisconnected);

  // Service UUID and MCUi marker in the advertisement, name in the scan response
  static const uint8_t marker[] = {'M', 'C', 'U', 'i', MCUI_PROTOCOL_VERSION};
  BLEAdvertisingData adv;
  adv.setAdvertisedService(g_service);
  adv.setManufacturerData(0xFFFF, marker, sizeof(marker));
  BLE.setAdvertisingData(adv);
  BLEAdvertisingData scan;
  scan.setLocalName(deviceName);
  BLE.setScanResponseData(scan);
  BLE.advertise();
  return true;
}

void bleSend(const char *line, size_t len, uint16_t chunk) {
  size_t total = len + 1;  // with '\n'
  size_t pos = 0;
  uint8_t buf[244];
  while (pos < total && BLE.connected()) {
    size_t n = total - pos;
    if (n > chunk) n = chunk;
    if (n > sizeof(buf)) n = sizeof(buf);
    for (size_t i = 0; i < n; i++) {
      size_t p = pos + i;
      buf[i] = p < len ? (uint8_t)line[p] : (uint8_t)'\n';
    }
    g_tx.writeValue(buf, n);
    pos += n;
    BLE.poll();
    delay(3);
  }
}

void blePoll(MCUi *) { BLE.poll(); }

}  // namespace mcui_internal

#else  // no Bluetooth LE: ESP32-S2, ESP8266, AVR, STM32, ... (or ArduinoBLE not included)

namespace mcui_internal {
bool bleBegin(MCUi *, const char *) { return false; }
void bleSend(const char *, size_t, uint16_t) {}
void blePoll(MCUi *) {}
}  // namespace mcui_internal

#endif
