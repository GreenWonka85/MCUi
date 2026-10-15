/*
  MCUi - internal interface between the protocol code and the connections.
  Not meant to be used by sketches.
  License: MIT
*/
#pragma once

#include <Arduino.h>

class MCUi;

namespace mcui_internal {

// Bluetooth LE (MCUi_BLE.cpp): ESP32 built in, other boards with ArduinoBLE
bool bleBegin(MCUi *bench, const char *deviceName);
/** Sends one line plus '\n', split into packets of at most `chunk` bytes. */
void bleSend(const char *line, size_t len, uint16_t chunk);
/** Lets the BLE stack work (ArduinoBLE needs BLE.poll()). */
void blePoll(MCUi *bench);

// Bluetooth Classic (MCUi_Classic.cpp)
bool classicBegin(MCUi *bench, const char *deviceName);
void classicPoll(MCUi *bench);
void classicSend(const char *line, size_t len);

// Wi-Fi (MCUi_WiFi.cpp)
bool wifiBegin(MCUi *bench, const char *deviceName, const char *ssid, const char *password);
bool wifiBeginAP(MCUi *bench, const char *ssid, const char *password);
void wifiPoll(MCUi *bench);
void wifiSend(const char *line, size_t len);

}  // namespace mcui_internal
