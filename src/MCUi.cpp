/*
  MCUi - protocol and channel handling. The connections are in
  MCUi_BLE.cpp, MCUi_Classic.cpp and MCUi_WiFi.cpp.
  License: MIT
*/
#include "MCUi.h"
#include "MCUi_internal.h"
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------------- helpers

static void appendf(char *buf, size_t size, const char *fmt, ...) {
  size_t used = strlen(buf);
  if (used >= size - 1) return;
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf + used, size - used, fmt, args);
  va_end(args);
}

static void formatFloat(char *out, size_t size, float value, uint8_t decimals) {
  if (isnan(value)) {
    snprintf(out, size, "nan");
  } else {
    snprintf(out, size, "%.*f", decimals, value);
  }
}

static uint8_t decimalsForStep(float step) {
  if (step <= 0 || step >= 1) return 0;
  uint8_t d = 0;
  float s = step;
  while (s < 0.999f && d < 6) {
    s *= 10;
    d++;
  }
  return d;
}

// ---------------------------------------------------------------- setup

MCUi::MCUi() {
  memset(_channels, 0, sizeof(_channels));
  memset(_controls, 0, sizeof(_controls));
  memset(_options, 0, sizeof(_options));
  memset(_id, 0, sizeof(_id));
}

void MCUi::initCommon(const char *deviceName, Link link) {
  _name = deviceName;
  _link = link;
  uint64_t mac = ESP.getEfuseMac();
  snprintf(_id, sizeof(_id), "%04X%08X", (unsigned)(uint16_t)(mac >> 32), (unsigned)(uint32_t)mac);
}

bool MCUi::begin(const char *deviceName) {
  initCommon(deviceName, LINK_BLE);
  bool ok = mcui_internal::bleBegin(this, deviceName);
  if (_debug) {
    if (ok) _debug->printf("MCUi: Bluetooth LE ready, advertising as \"%s\" (ID %s)\n", deviceName, _id);
    else _debug->println("MCUi: this board has no Bluetooth LE");
  }
  return ok;
}

bool MCUi::beginClassic(const char *deviceName) {
  initCommon(deviceName, LINK_CLASSIC);
  bool ok = mcui_internal::classicBegin(this, deviceName);
  if (_debug) {
    if (ok) _debug->printf("MCUi: Bluetooth Classic ready as \"%s\" (ID %s). Pair it in the Android Bluetooth settings.\n", deviceName, _id);
    else _debug->println("MCUi: this board has no Bluetooth Classic (only the classic ESP32 has it)");
  }
  return ok;
}

bool MCUi::beginWiFi(const char *deviceName, const char *ssid, const char *password) {
  initCommon(deviceName, LINK_WIFI);
  return mcui_internal::wifiBegin(this, deviceName, ssid, password);
}

bool MCUi::beginWiFi(const char *deviceName) {
  return beginWiFi(deviceName, nullptr, nullptr);
}

void MCUi::setFirmware(const char *version) { _firmware = version; }

void MCUi::setInterval(uint32_t ms) { _interval = ms < 20 ? 20 : ms; }

void MCUi::setDebug(Print &out) { _debug = &out; }

// ---------------------------------------------------------------- channels

bool MCUi::addChannel(const char *key, const char *name, const char *type, const char *unit) {
  if (_channelCount >= MCUI_MAX_CHANNELS || findChannel(key)) return false;
  Channel &c = _channels[_channelCount++];
  memset(&c, 0, sizeof(Channel));
  c.key = key;
  c.name = name;
  c.type = type;
  c.unit = unit;
  c.decimals = 255;
  c.value = NAN;
  c.isText = strcmp(type, "psu_mode") == 0 || strcmp(type, "text") == 0;
  return true;
}

void MCUi::setRange(const char *key, float min, float max) {
  Channel *c = findChannel(key);
  if (!c) return;
  c->min = min;
  c->max = max;
  c->hasRange = true;
}

void MCUi::setDecimals(const char *key, uint8_t decimals) {
  Channel *c = findChannel(key);
  if (c) c->decimals = decimals > 9 ? 9 : decimals;
}

void MCUi::setGroup(const char *key, const char *group) {
  Channel *c = findChannel(key);
  if (c) c->group = group;
}

void MCUi::setPage(const char *key, uint8_t page) {
  Channel *c = findChannel(key);
  if (c) c->page = page;
}

void MCUi::setSetpointOf(const char *key, const char *measuredKey) {
  Channel *c = findChannel(key);
  if (c) c->setpointOf = measuredKey;
}

void MCUi::setAlarm(const char *key, float low, float high) {
  Channel *c = findChannel(key);
  if (!c) return;
  c->alarmLow = low;
  c->alarmHigh = high;
  c->hasAlarm = true;
}

// ---------------------------------------------------------------- controls

bool MCUi::addSwitch(const char *key, const char *name, bool initial) {
  if (_controlCount >= MCUI_MAX_CONTROLS || findControl(key)) return false;
  Control &c = _controls[_controlCount++];
  memset(&c, 0, sizeof(Control));
  c.key = key;
  c.name = name;
  c.type = "switch";
  strcpy(c.value, initial ? "1" : "0");
  return true;
}

bool MCUi::addButton(const char *key, const char *name) {
  if (_controlCount >= MCUI_MAX_CONTROLS || findControl(key)) return false;
  Control &c = _controls[_controlCount++];
  memset(&c, 0, sizeof(Control));
  c.key = key;
  c.name = name;
  c.type = "button";
  strcpy(c.value, "0");
  return true;
}

bool MCUi::addSlider(const char *key, const char *name, float min, float max, float step, float initial, const char *unit) {
  if (_controlCount >= MCUI_MAX_CONTROLS || findControl(key)) return false;
  Control &c = _controls[_controlCount++];
  memset(&c, 0, sizeof(Control));
  c.key = key;
  c.name = name;
  c.type = "slider";
  c.min = min;
  c.max = max;
  c.step = step;
  c.unit = unit;
  formatFloat(c.value, sizeof(c.value), initial, decimalsForStep(step));
  return true;
}

bool MCUi::addNumber(const char *key, const char *name, float min, float max, float initial, const char *unit) {
  if (_controlCount >= MCUI_MAX_CONTROLS || findControl(key)) return false;
  Control &c = _controls[_controlCount++];
  memset(&c, 0, sizeof(Control));
  c.key = key;
  c.name = name;
  c.type = "number";
  c.min = min;
  c.max = max;
  c.unit = unit;
  snprintf(c.value, sizeof(c.value), "%g", initial);
  return true;
}

bool MCUi::addSelect(const char *key, const char *name, const char *options, const char *initial) {
  if (_controlCount >= MCUI_MAX_CONTROLS || findControl(key)) return false;
  Control &c = _controls[_controlCount++];
  memset(&c, 0, sizeof(Control));
  c.key = key;
  c.name = name;
  c.type = "select";
  c.options = options;
  strncpy(c.value, initial ? initial : "", sizeof(c.value) - 1);
  return true;
}

void MCUi::setControlGroup(const char *key, const char *group) {
  Control *c = findControl(key);
  if (c) c->group = group;
}

void MCUi::setControlPage(const char *key, uint8_t page) {
  Control *c = findControl(key);
  if (c) c->page = page;
}

void MCUi::onControl(ControlCallback callback) { _callback = callback; }

bool MCUi::controlBool(const char *key) {
  Control *c = findControl(key);
  if (!c || strcmp(c->value, "1") != 0) return false;
  // A button press is reported once, then cleared
  if (strcmp(c->type, "button") == 0) strcpy(c->value, "0");
  return true;
}

float MCUi::controlFloat(const char *key) {
  Control *c = findControl(key);
  return c ? atof(c->value) : NAN;
}

const char *MCUi::controlText(const char *key) {
  Control *c = findControl(key);
  return c ? c->value : "";
}

void MCUi::setControl(const char *key, const char *value) {
  Control *c = findControl(key);
  if (!c) return;
  strncpy(c->value, value, sizeof(c->value) - 1);
  c->value[sizeof(c->value) - 1] = 0;
  if (_announced) {
    char line[64];
    snprintf(line, sizeof(line), "ACK:%s=%s", c->key, c->value);
    sendLine(line);
  }
}

void MCUi::setControl(const char *key, float value) {
  Control *c = findControl(key);
  if (!c) return;
  char buf[24];
  formatFloat(buf, sizeof(buf), value, decimalsForStep(c->step));
  setControl(key, buf);
}

void MCUi::setOption(const char *name, const char *value) {
  for (uint8_t i = 0; i < _optionCount; i++) {
    if (strcmp(_options[i][0], name) == 0) {
      _options[i][1] = value;
      return;
    }
  }
  if (_optionCount < 4) {
    _options[_optionCount][0] = name;
    _options[_optionCount][1] = value;
    _optionCount++;
  }
}

// ---------------------------------------------------------------- values

void MCUi::set(const char *key, float value) {
  Channel *c = findChannel(key);
  if (!c) return;
  c->value = value;
  c->hasValue = true;
}

void MCUi::setText(const char *key, const char *text) {
  Channel *c = findChannel(key);
  if (!c) return;
  strncpy(c->text, text, sizeof(c->text) - 1);
  c->text[sizeof(c->text) - 1] = 0;
  c->isText = true;
  c->hasValue = true;
}

void MCUi::event(const char *text) {
  if (!_announced) return;
  char line[160];
  snprintf(line, sizeof(line), "EVT:%s", text);
  sendLine(line);
}

bool MCUi::isConnected() const { return _connected; }

// ---------------------------------------------------------------- runtime

void MCUi::_onConnect() {
  _connected = true;
  _connectEvent = true;
}

void MCUi::_onDisconnect() {
  _connected = false;
  _connectEvent = true;
}

void MCUi::_onReceive(const uint8_t *data, size_t len) {
  portENTER_CRITICAL(&g_mux);
  for (size_t i = 0; i < len; i++) {
    uint16_t next = (_rxHead + 1) % RX_SIZE;
    if (next == _rxTail) break; // buffer full
    _rx[_rxHead] = data[i];
    _rxHead = next;
  }
  portEXIT_CRITICAL(&g_mux);
}

void MCUi::update() {
  // Classic and Wi-Fi are polled here, Bluetooth LE uses callbacks
  if (_link == LINK_CLASSIC) mcui_internal::classicPoll(this);
  else if (_link == LINK_WIFI) mcui_internal::wifiPoll(this);

  uint32_t now = millis();

  if (_connectEvent) {
    _connectEvent = false;
    if (_connected) {
      _connectedAt = now;
      _announced = false;
      _helloSeen = false;
      _chunk = 20;
      _lineLen = 0;
      if (_debug) _debug->println("MCUi: app connected");
    } else {
      _announced = false;
      if (_debug) _debug->println("MCUi: app disconnected, waiting for the app");
    }
  }

  // Received bytes into lines
  while (true) {
    uint8_t b;
    portENTER_CRITICAL(&g_mux);
    bool empty = _rxHead == _rxTail;
    if (!empty) {
      b = _rx[_rxTail];
      _rxTail = (_rxTail + 1) % RX_SIZE;
    }
    portEXIT_CRITICAL(&g_mux);
    if (empty) break;

    if (b == '\n') {
      _line[_lineLen] = 0;
      if (_lineLen > 0) handleLine(_line);
      _lineLen = 0;
    } else if (b != '\r' && _lineLen < sizeof(_line) - 1) {
      _line[_lineLen++] = (char)b;
    }
  }

  if (!_connected) return;

  // No HELLO from the app: send the announcement on our own (protocol 2.)
  if (!_announced && now - _connectedAt > 2000) sendAnnouncement();

  if (_announced && now - _lastSend >= _interval) sendNow();
}

void MCUi::handleLine(char *line) {
  if (strncmp(line, "HELLO", 5) == 0) {
    const char *mtu = strstr(line, "mtu=");
    if (mtu) {
      int m = atoi(mtu + 4);
      int chunk = m - 3;
      if (chunk < 20) chunk = 20;
      if (chunk > 240) chunk = 240;
      _chunk = (uint16_t)chunk;
    }
    _helloSeen = true;
    if (_debug) _debug->printf("MCUi: HELLO from app, packet size %u\n", _chunk);
    sendAnnouncement();
    return;
  }

  if (strcmp(line, "PING") == 0) {
    sendLine("PONG");
    return;
  }

  if (strncmp(line, "SET:", 4) == 0) {
    char *body = line + 4;
    char *eq = strchr(body, '=');
    if (!eq) return;
    *eq = 0;
    const char *key = body;
    const char *value = eq + 1;
    char reply[96];

    Control *c = findControl(key);
    if (!c) {
      snprintf(reply, sizeof(reply), "ERR:%s=unknown control", key);
      sendLine(reply);
      return;
    }

    if (strcmp(c->type, "switch") == 0) {
      strcpy(c->value, (strcmp(value, "1") == 0 || strcasecmp(value, "on") == 0) ? "1" : "0");
    } else if (strcmp(c->type, "button") == 0) {
      strcpy(c->value, "1");
    } else if (strcmp(c->type, "slider") == 0 || strcmp(c->type, "number") == 0) {
      float f = atof(value);
      if (f < c->min) f = c->min;
      if (f > c->max) f = c->max;
      if (strcmp(c->type, "slider") == 0) {
        formatFloat(c->value, sizeof(c->value), f, decimalsForStep(c->step));
      } else {
        snprintf(c->value, sizeof(c->value), "%g", f);
      }
    } else if (strcmp(c->type, "select") == 0) {
      // Check that the value is one of the options
      bool ok = false;
      const char *p = c->options;
      size_t vlen = strlen(value);
      while (p && *p) {
        const char *end = strchr(p, '|');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len == vlen && strncmp(p, value, len) == 0) {
          ok = true;
          break;
        }
        p = end ? end + 1 : nullptr;
      }
      if (!ok) {
        snprintf(reply, sizeof(reply), "ERR:%s=unknown option", key);
        sendLine(reply);
        return;
      }
      strncpy(c->value, value, sizeof(c->value) - 1);
      c->value[sizeof(c->value) - 1] = 0;
    }

    if (_callback) _callback(c->key, c->value);
    snprintf(reply, sizeof(reply), "ACK:%s=%s", c->key, c->value);
    sendLine(reply);
  }
}

void MCUi::sendAnnouncement() {
  char line[320];

  snprintf(line, sizeof(line), "DEV:id=%s;name=%s;proto=%d", _id, _name, MCUI_PROTOCOL_VERSION);
  if (_firmware) appendf(line, sizeof(line), ";fw=%s", _firmware);
  sendLine(line);

  for (uint8_t i = 0; i < _channelCount; i++) {
    Channel &c = _channels[i];
    snprintf(line, sizeof(line), "CH:key=%s;name=%s;type=%s", c.key, c.name ? c.name : c.key, c.type);
    if (c.unit) appendf(line, sizeof(line), ";unit=%s", c.unit);
    if (c.hasRange) appendf(line, sizeof(line), ";min=%g;max=%g", c.min, c.max);
    if (c.decimals != 255) appendf(line, sizeof(line), ";dec=%u", c.decimals);
    if (c.group) appendf(line, sizeof(line), ";group=%s", c.group);
    if (c.page) appendf(line, sizeof(line), ";page=%u", c.page);
    if (c.setpointOf) appendf(line, sizeof(line), ";role=setpoint;of=%s", c.setpointOf);
    if (c.hasAlarm) appendf(line, sizeof(line), ";alarm_lo=%g;alarm_hi=%g", c.alarmLow, c.alarmHigh);
    sendLine(line);
  }

  for (uint8_t i = 0; i < _controlCount; i++) {
    Control &c = _controls[i];
    snprintf(line, sizeof(line), "CTL:key=%s;name=%s;type=%s", c.key, c.name ? c.name : c.key, c.type);
    if (strcmp(c.type, "slider") == 0) {
      appendf(line, sizeof(line), ";min=%g;max=%g;step=%g", c.min, c.max, c.step);
    } else if (strcmp(c.type, "number") == 0) {
      appendf(line, sizeof(line), ";min=%g;max=%g", c.min, c.max);
    } else if (strcmp(c.type, "select") == 0 && c.options) {
      appendf(line, sizeof(line), ";options=%s", c.options);
    }
    if (c.unit) appendf(line, sizeof(line), ";unit=%s", c.unit);
    if (strcmp(c.type, "button") != 0) appendf(line, sizeof(line), ";value=%s", c.value);
    if (c.group) appendf(line, sizeof(line), ";group=%s", c.group);
    if (c.page) appendf(line, sizeof(line), ";page=%u", c.page);
    sendLine(line);
  }

  if (_optionCount > 0) {
    snprintf(line, sizeof(line), "OPT:");
    for (uint8_t i = 0; i < _optionCount; i++) {
      appendf(line, sizeof(line), "%s%s=%s", i ? ";" : "", _options[i][0], _options[i][1]);
    }
    sendLine(line);
  }

  sendLine("END");
  _announced = true;
  _lastSend = 0;
}

void MCUi::sendNow() {
  if (!_connected || !_announced) return;
  char line[512];
  line[0] = 0;
  char num[24];
  for (uint8_t i = 0; i < _channelCount; i++) {
    Channel &c = _channels[i];
    if (!c.hasValue) continue;
    if (c.isText) {
      appendf(line, sizeof(line), "%s%s=%s", line[0] ? ";" : "", c.key, c.text);
    } else {
      formatFloat(num, sizeof(num), c.value, c.decimals != 255 ? c.decimals + 1 : defaultDecimals(c.type));
      appendf(line, sizeof(line), "%s%s=%s", line[0] ? ";" : "", c.key, num);
    }
  }
  _lastSend = millis();
  if (line[0]) sendLine(line);
}

void MCUi::sendLine(const char *line) {
  if (!_connected) return;
  size_t len = strlen(line);
  switch (_link) {
    case LINK_BLE: mcui_internal::bleSend(line, len, _chunk); break;
    case LINK_CLASSIC: mcui_internal::classicSend(line, len); break;
    case LINK_WIFI: mcui_internal::wifiSend(line, len); break;
    default: break;
  }
}

// ---------------------------------------------------------------- lookup

MCUi::Channel *MCUi::findChannel(const char *key) {
  for (uint8_t i = 0; i < _channelCount; i++) {
    if (strcmp(_channels[i].key, key) == 0) return &_channels[i];
  }
  return nullptr;
}

MCUi::Control *MCUi::findControl(const char *key) {
  for (uint8_t i = 0; i < _controlCount; i++) {
    if (strcmp(_controls[i].key, key) == 0) return &_controls[i];
  }
  return nullptr;
}

uint8_t MCUi::defaultDecimals(const char *type) {
  if (strcmp(type, "current") == 0) return 4;
  if (strcmp(type, "voltage") == 0) return 3;
  if (strcmp(type, "capacitance") == 0 || strcmp(type, "inductance") == 0) return 9;
  return 3;
}
