/*
  MCUi - show measurements from your microcontroller in the MCU Instruments app.

  Protocol: docs/PROTOCOL.md
  License:  MIT

  Version 0.2: Bluetooth LE, Bluetooth Classic and Wi-Fi on ESP32.

    begin("Name")                     Bluetooth LE   ESP32, -S3, -C3, -C6
    beginClassic("Name")              Bluetooth      ESP32 only (Android only)
    beginWiFi("Name", ssid, password) Wi-Fi          all ESP32 incl. -S2 (existing network)
    beginWiFiAP("Name", password)     Wi-Fi          all ESP32 incl. -S2 (own network)

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
#error "MCUi supports ESP32 boards only. Other boards follow in a later version."
#endif

// ESP32-S3/C3/C6 with "USB CDC On Boot" on core 2.x: writes to the USB serial
// port block while no Serial Monitor is open. Bluetooth then stalls as well.
// Fixed in core 3.0.0 (see espressif/arduino-esp32 PR #7721).
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT && defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE && \
    (!defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3)
#warning "MCUi: ESP32 core 2.x with USB CDC On Boot: the sketch stalls without an open Serial Monitor. Please update the ESP32 core to 3.x or set Tools > USB CDC On Boot > Disabled."
#endif

#ifndef MCUI_MAX_CHANNELS
#define MCUI_MAX_CHANNELS 24
#endif

#ifndef MCUI_MAX_CONTROLS
#define MCUI_MAX_CONTROLS 16
#endif

#define MCUI_PROTOCOL_VERSION 1
#define MCUI_LIBRARY_VERSION "0.2.0"

#ifndef MCUI_WIFI_PORT
#define MCUI_WIFI_PORT 4210
#endif

class MCUi {
public:
  /** Called when the app changes a control. value is the new value as text. */
  typedef void (*ControlCallback)(const char *key, const char *value);

  MCUi();

  // ------------------------------------------------------------ Setup
  // Start ONE of the connections below, after defining channels and controls.

  /** Bluetooth LE (ESP32, ESP32-S3, -C3, -C6). Works with Android and iOS. */
  bool begin(const char *deviceName);

  /**
   * Bluetooth Classic, Serial Port Profile (classic ESP32 only, Android only).
   * Pair the ESP32 once in the Android Bluetooth settings.
   */
  bool beginClassic(const char *deviceName);

  /**
   * Wi-Fi: connects to your network and waits for the app on TCP port 4210.
   * The app finds the device automatically (mDNS) or by its IP address.
   */
  bool beginWiFi(const char *deviceName, const char *ssid, const char *password);

  /** Wi-Fi, if your sketch already connects to the network itself. */
  bool beginWiFi(const char *deviceName);

  /**
   * Wi-Fi access point: the ESP32 opens its own network, no router needed.
   * Connect the phone to this network; the app reaches the ESP32 at 192.168.4.1.
   * password: at least 8 characters, or nullptr for an open network.
   * ssid: network name, default is the device name.
   */
  bool beginWiFiAP(const char *deviceName, const char *password = nullptr, const char *ssid = nullptr);

  /** Firmware version shown in the app (optional). */
  void setFirmware(const char *version);

  /** Data send interval in milliseconds (default 200). */
  void setInterval(uint32_t ms);

  /**
   * Prints status messages (advertising, connected, disconnected) to a
   * serial port, e.g. setDebug(Serial). Call before begin().
   */
  void setDebug(Print &out);

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

  // Internal: called by the connection code
  const char *_deviceId() const { return _id; }
  const char *_deviceName() const { return _name; }
  Print *_debugOut() const { return _debug; }
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

  enum Link : uint8_t { LINK_NONE, LINK_BLE, LINK_CLASSIC, LINK_WIFI };

  void initCommon(const char *deviceName, Link link);
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
  Link _link = LINK_NONE;
  ControlCallback _callback = nullptr;
  Print *_debug = nullptr;

  volatile bool _connected = false;
  volatile bool _connectEvent = false;
  bool _announced = false;
  bool _helloSeen = false;

  // Receive ring buffer (filled from the Bluetooth task or update())
  static const uint16_t RX_SIZE = 512;
  uint8_t _rx[RX_SIZE];
  volatile uint16_t _rxHead = 0;
  volatile uint16_t _rxTail = 0;
  char _line[256];
  uint16_t _lineLen = 0;
};
