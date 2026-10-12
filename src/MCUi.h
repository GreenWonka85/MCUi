/*
  MCUi - show measurements from your microcontroller in the MCU Instruments app.

  Protocol: docs/PROTOCOL.md
  License:  MIT

  Version 0.1: Bluetooth LE on ESP32 (ESP32, ESP32-S3, ESP32-C3, ESP32-C6).
  Bluetooth Classic and Wi-Fi follow in a later version.

  Minimal example:

    #include <MCUi.h>
    MCUi mcui;

    void setup() {
      mcui.addChannel("V", "Voltage", "voltage");
      mcui.setRange("V", 0, 30);
      mcui.begin("My Voltmeter");
    }

    void loop() {
      mcui.set("V", readVoltage());
      mcui.update();
    }

  Important: keys, names, units, groups and option lists are stored as
  pointers. Use string literals ("V") or buffers that stay valid
  (global or static), not temporary Strings.
*/
#pragma once

#include <Arduino.h>

#if !defined(ESP32)
#error "MCUi 0.1 supports ESP32 boards only. Other boards follow in a later version."
#endif

#ifndef MCUI_MAX_CHANNELS
#define MCUI_MAX_CHANNELS 24
#endif

#ifndef MCUI_MAX_CONTROLS
#define MCUI_MAX_CONTROLS 16
#endif

#define MCUI_PROTOCOL_VERSION 1
#define MCUI_LIBRARY_VERSION "0.1.0"

class MCUi {
public:
  /** Called when the app changes a control. value is the new value as text. */
  typedef void (*ControlCallback)(const char *key, const char *value);

  MCUi();

  // ------------------------------------------------------------ Setup
  /** Starts Bluetooth LE. Call after defining channels and controls. */
  bool begin(const char *deviceName);

  /** Firmware version shown in the app (optional). */
  void setFirmware(const char *version);

  /** Data send interval in milliseconds (default 200). */
  void setInterval(uint32_t ms);

  // ------------------------------------------------------------ Channels
  /**
   * Adds a measurement channel.
   * type: "voltage", "current", "temperature", "humidity", "pressure", ...
   *       (see docs/PROTOCOL.md, section 5). Unit and range default to the type.
   */
  bool addChannel(const char *key, const char *name, const char *type, const char *unit = nullptr);

  void setRange(const char *key, float min, float max);
  void setDecimals(const char *key, uint8_t decimals);
  /** Channels with the same group belong together, e.g. "K1" and "K2" of a dual power supply. */
  void setGroup(const char *key, const char *group);
  /** Preferred page in the app (1, 2, ...). */
  void setPage(const char *key, uint8_t page);
  /** Marks a channel as setpoint of another channel (e.g. voltage setpoint of "V"). */
  void setSetpointOf(const char *key, const char *measuredKey);
  /** Default alarm limits shown in the app. */
  void setAlarm(const char *key, float low, float high);

  // ------------------------------------------------------------ Controls
  bool addSwitch(const char *key, const char *name, bool initial = false);
  bool addButton(const char *key, const char *name);
  bool addSlider(const char *key, const char *name, float min, float max, float step, float initial, const char *unit = nullptr);
  bool addNumber(const char *key, const char *name, float min, float max, float initial, const char *unit = nullptr);
  /** options: "AUTO|4V|40V" */
  bool addSelect(const char *key, const char *name, const char *options, const char *initial);
  void setControlGroup(const char *key, const char *group);
  void setControlPage(const char *key, uint8_t page);

  void onControl(ControlCallback callback);

  /**
   * Current control values (as last set by the app or by your code).
   * For a button, controlBool() returns true once per press.
   */
  bool controlBool(const char *key);
  float controlFloat(const char *key);
  const char *controlText(const char *key);
  /** Changes a control value from the device side and informs the app. */
  void setControl(const char *key, const char *value);
  void setControl(const char *key, float value);

  // ------------------------------------------------------------ Options
  /** Display options, e.g. setOption("cube", "1") for the 3D cube. */
  void setOption(const char *name, const char *value);

  // ------------------------------------------------------------ Values
  void set(const char *key, float value);
  void setText(const char *key, const char *text);
  /** Shows a message in the app. */
  void event(const char *text);

  // ------------------------------------------------------------ Runtime
  /** Call as often as possible in loop(). Handles commands and sends data. */
  void update();
  /** Sends the current values immediately. */
  void sendNow();
  bool isConnected() const;

  // Internal: called by the Bluetooth callbacks
  void _onConnect();
  void _onDisconnect();
  void _onReceive(const uint8_t *data, size_t len);

private:
  struct Channel {
    const char *key;
    const char *name;
    const char *type;
    const char *unit;
    const char *group;
    const char *setpointOf;
    float min, max, alarmLow, alarmHigh, value;
    uint8_t decimals; // 255 = default
    uint8_t page;     // 0 = none
    bool hasRange, hasAlarm, hasValue, isText;
    char text[16];
  };

  struct Control {
    const char *key;
    const char *name;
    const char *type;
    const char *unit;
    const char *options;
    const char *group;
    float min, max, step;
    uint8_t page;
    char value[24];
  };

  Channel *findChannel(const char *key);
  Control *findControl(const char *key);
  void sendLine(const char *line);
  void sendAnnouncement();
  void handleLine(char *line);
  static uint8_t defaultDecimals(const char *type);

  Channel _channels[MCUI_MAX_CHANNELS];
  Control _controls[MCUI_MAX_CONTROLS];
  uint8_t _channelCount = 0;
  uint8_t _controlCount = 0;
  const char *_options[4][2];
  uint8_t _optionCount = 0;

  const char *_name = "MCU Instruments";
  const char *_firmware = nullptr;
  char _id[13];
  uint32_t _interval = 200;
  uint32_t _lastSend = 0;
  uint32_t _connectedAt = 0;
  uint16_t _chunk = 20;
  ControlCallback _callback = nullptr;

  volatile bool _connected = false;
  volatile bool _connectEvent = false;
  bool _announced = false;
  bool _helloSeen = false;

  // Receive ring buffer (filled from the Bluetooth task)
  static const uint16_t RX_SIZE = 512;
  uint8_t _rx[RX_SIZE];
  volatile uint16_t _rxHead = 0;
  volatile uint16_t _rxTail = 0;
  char _line[256];
  uint16_t _lineLen = 0;
};
