# MCU Instruments Protocol, Version 1

This document describes how a microcontroller talks to the MCU Instruments app.
The protocol is plain text, line based and identical on every transport
(Bluetooth LE, Bluetooth Classic, Wi-Fi). Any device that follows it works
with the app, with or without the MCU Instruments Arduino library.

Status: draft 1, October 2026.

## 1. Basics

- Encoding: UTF-8.
- Every message is one line terminated by `\n` (LF). A preceding `\r` is ignored.
- Maximum line length: 512 bytes. Longer lines are discarded by the app.
- Fields are separated by `;`, keys and values by `=`.
- Names and values must not contain `;`, `=` or line breaks.
- Decimal separator for numbers is always `.` (independent of the phone language).
- Keys are case sensitive, 1 to 16 characters, letters, digits and `_`.
- Empty lines and lines starting with `#` are ignored (comments, debug output).

## 2. Connection sequence

```
App                                   Device
 |  (transport connected)               |
 |---- HELLO proto=1;mtu=247 ---------->|
 |<--- DEV:id=...;name=...;proto=1 -----|
 |<--- CH:... (one line per channel) ---|
 |<--- CTL:... (one line per control) --|
 |<--- OPT:... (optional) --------------|
 |<--- END -----------------------------|
 |<--- V=12.03;I=0.251;M=CV ------------|  data, repeatedly
 |---- SET:OUT=1 ---------------------->|
 |<--- ACK:OUT=1 -----------------------|
```

- The app sends `HELLO` right after the connection is ready.
- The device answers with its announcement block (`DEV` to `END`).
- The device may resend the announcement at any time, for example after the
  firmware added a channel. The app then rebuilds its pages.
- If no `HELLO` arrives within 2 seconds, the device should send the
  announcement on its own.

## 3. Device to app

### 3.1 Device info

```
DEV:id=A4CF12B37E90;name=Lab PSU 30V;proto=1;fw=1.0.0
```

| Field | Required | Meaning |
|---|---|---|
| `id` | yes | Unique, stable device ID, 4 to 32 characters. The library uses the chip ID. The app stores layouts and recordings per `id`. |
| `name` | yes | Display name. |
| `proto` | yes | Protocol version, currently `1`. |
| `fw` | no | Firmware version, shown in the app. |

### 3.2 Channel definition

```
CH:key=V1;name=Voltage;unit=V;type=voltage;min=0;max=30;dec=2;group=K1;page=1
```

| Field | Required | Meaning |
|---|---|---|
| `key` | yes | Short key used in data lines. |
| `name` | no | Display name. Default: the key. |
| `type` | no | Channel type, see section 5. Default: `generic`. |
| `unit` | no | Unit text, for example `V`, `°C`, `hPa`. Default: the unit of the type. |
| `min`, `max` | no | Scale range for analog gauges and bars. Default: from the type. |
| `dec` | no | Number of decimals to display. Default: from the type. |
| `group` | no | Channels with the same group belong together, for example both channels of a dual power supply (`K1`, `K2`). |
| `page` | no | Preferred page number (1, 2, ...). Only a hint, the user can rearrange. |
| `role` | no | `value` (default) or `setpoint`. |
| `of` | no | For `role=setpoint`: key of the measured channel this setpoint belongs to. |
| `alarm_lo`, `alarm_hi` | no | Default alarm limits. |

### 3.3 Control definition (app can change something on the device)

```
CTL:key=OUT;name=Output;type=switch;value=0
CTL:key=VSET;name=Voltage;type=slider;min=0;max=30;step=0.01;unit=V;value=12.00;group=K1
CTL:key=ZERO;name=Zero;type=button
CTL:key=RANGE;name=Range;type=select;options=AUTO|4V|40V|400V;value=AUTO
CTL:key=FREQ;name=Frequency;type=number;min=1;max=100000;unit=Hz;value=1000
```

| Type | Meaning | Value sent by the app |
|---|---|---|
| `switch` | On/off | `0` or `1` |
| `button` | Momentary action | `1` |
| `slider` | Continuous value with `min`, `max`, `step` | number |
| `number` | Numeric input with optional `min`, `max` | number |
| `select` | One of `options` (separated by `|`) | option text |

Optional fields: `unit`, `value` (current value), `group`, `page`.

### 3.4 Options

```
OPT:cube=1
```

| Option | Meaning |
|---|---|
| `cube=1` | Show the 3D cube on the orientation page (gyroscope). |
| `interval=200` | Typical send interval in ms, used by the app for timeouts. |

### 3.5 End of announcement

```
END
```

### 3.6 Data

A data line contains one or more `key=value` pairs and no prefix:

```
V1=12.034;I1=0.251;M1=CV;T=42.1
t=123456;V1=12.034
```

- Numeric channels: decimal number, `nan` for "no valid value".
- Text and state channels (`psu_mode`, `state`, `text`): the text, for example `CV`, `CC`, `OFF`.
- Optional `t=` is the device time in milliseconds. If present, recordings use it
  instead of the arrival time on the phone.
- A data line may contain any subset of the channels. Missing channels keep their last value.

### 3.7 Acknowledgement and messages

```
ACK:OUT=1          control value accepted (and now active)
ERR:VSET=out of range
EVT:Overtemperature on channel 1
```

`EVT` lines are shown as a notice in the app and stored as events in recordings.

## 4. App to device

| Line | Meaning |
|---|---|
| `HELLO proto=1;mtu=247` | Request announcement. `mtu` is the negotiated BLE MTU, the device may send up to `mtu - 3` bytes per notification. |
| `SET:key=value` | Change a control. The device should answer with `ACK` or `ERR`. |
| `PING` | Optional keep-alive. The device may answer `PONG`. |

## 5. Channel types

| Type | Default unit | Default range | Decimals | Typical display |
|---|---|---|---|---|
| `voltage` | V | 0 to 30 | 2 | analog gauge, digital |
| `current` | A | 0 to 5 | 3 | analog gauge, digital |
| `power` | W | 0 to 100 | 2 | digital |
| `energy` | Wh | 0 to 1000 | 2 | digital |
| `resistance` | Ω | 0 to 1000 | 2 | digital |
| `capacitance` | F | 0 to 0.001 | 9 | digital |
| `inductance` | H | 0 to 0.001 | 9 | digital |
| `frequency` | Hz | 0 to 1000 | 1 | digital |
| `temperature` | °C | 0 to 100 | 1 | thermometer, tile |
| `humidity` | % | 0 to 100 | 0 | hygrometer, tile |
| `pressure` | hPa | 950 to 1050 | 0 | barometer, tile |
| `co2` | ppm | 0 to 2000 | 0 | bar |
| `pm` | µg/m³ | 0 to 100 | 0 | bar |
| `voc` | index | 0 to 500 | 0 | bar |
| `light` | lx | 0 to 10000 | 0 | tile |
| `uv` | index | 0 to 11 | 1 | bar |
| `distance` | cm | 0 to 400 | 1 | tile |
| `level` | % | 0 to 100 | 0 | tank |
| `weight` | kg | 0 to 5 | 3 | digital |
| `force` | N | 0 to 50 | 2 | digital |
| `rpm` | rpm | 0 to 8000 | 0 | tachometer |
| `flow` | l/min | 0 to 30 | 2 | tile |
| `speed` | km/h | 0 to 100 | 1 | tile |
| `angle` | ° | -180 to 180 | 1 | tile |
| `heading` | ° | 0 to 360 | 0 | compass |
| `roll`, `pitch`, `yaw` | ° | -180 to 180 | 1 | horizon, level |
| `accel_x`, `accel_y`, `accel_z` | g | -2 to 2 | 2 | axis bars |
| `gyro_x`, `gyro_y`, `gyro_z` | °/s | -250 to 250 | 1 | axis bars |
| `sound` | dB | 30 to 120 | 0 | bar |
| `percent` | % | 0 to 100 | 0 | bar |
| `count` | | 0 to 1000 | 0 | digital |
| `psu_mode` | | | | CV/CC indicator, values `CV`, `CC`, `OFF` |
| `state` | | | | status LED, `0`/`1` or text |
| `text` | | | | text line |
| `generic` | | 0 to 100 | 2 | tile |

Unknown types are displayed as `generic`.

## 6. Transports

### 6.1 Bluetooth LE (all platforms)

Nordic UART Service (NUS):

| | UUID |
|---|---|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (app writes) | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX (device notifies) | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

- The device advertises the NUS service UUID so the app can find it.
- It also advertises manufacturer data `FF FF 4D 43 55 69 01`
  (company ID 0xFFFF, "MCUi", protocol version). The app lists only devices with this marker.
- A line may be split over several notifications. The app reassembles at `\n`.
- Until `HELLO` with `mtu` has arrived, the device must not send more than 20 bytes per notification.

### 6.2 Bluetooth Classic (Android only)

Serial Port Profile (SPP), UUID `00001101-0000-1000-8000-00805F9B34FB`. Same lines as above.

- Only the classic ESP32 supports Bluetooth Classic. iOS does not allow SPP for apps.
- The device is paired once in the Android Bluetooth settings; the app lists paired devices.
- No packet size limit: `mtu` in `HELLO` is ignored.

### 6.3 Wi-Fi

- Two modes: the device joins an existing network (station), or opens its own
  network (access point) with the fixed address `192.168.4.1`.
- The device is a TCP server on port `4210` and accepts one app at a time.
  A new connection replaces the previous one (for example after the phone changed networks).
- Discovery via mDNS (DNS-SD): service `_mcui._tcp`, instance name = device name,
  TXT records `id` (device ID) and `name`. Host name `mcui-<last 6 hex digits of the ID>`.
- If mDNS is blocked by the network, the app connects by IP address.
- Same lines as above, no packet size limit.
- The app sends `PING` every 3 s. If no line arrives for 10 s, the app treats
  the connection as lost and reconnects.

## 7. Example: dual power supply

```
DEV:id=A4CF12B37E90;name=Dual PSU 2x30V;proto=1;fw=1.0.0
CH:key=V1;name=Voltage;type=voltage;max=30;group=K1
CH:key=I1;name=Current;type=current;max=3;group=K1
CH:key=M1;name=Mode;type=psu_mode;group=K1
CH:key=V2;name=Voltage;type=voltage;max=30;group=K2
CH:key=I2;name=Current;type=current;max=3;group=K2
CH:key=M2;name=Mode;type=psu_mode;group=K2
CTL:key=OUT;name=Outputs;type=switch;value=1
END
V1=12.00;I1=0.250;M1=CV;V2=3.30;I2=1.000;M2=CC
```
