# MCUi

Arduino library for the **MCU Instruments** app (short: MCUi): show measurements from your microcontroller on your phone or tablet as analog gauges, digital displays, power supply panels and sensor tiles, and control your device from the app.

*Deutsch: siehe unten.*

## Features (version 0.3)

- Works with (almost) every board:

  | Board | Connection | Call |
  |---|---|---|
  | ESP32, ESP32-S3, -C3, -C6 | Bluetooth LE built in | `begin("Name")` |
  | ESP32 (classic) | Bluetooth Classic built in (Android) | `beginClassic("Name")` |
  | ESP32 (all), ESP8266 (NodeMCU, Wemos D1 mini, ESP-01) | Wi-Fi in your network | `beginWiFi("Name", ssid, password)` |
  | ESP32 (all), ESP8266 | own Wi-Fi network (192.168.4.1) | `beginWiFiAP("Name", password)` |
  | Arduino Uno, Nano, Mega, Leonardo, STM32, RP2040, ... | Bluetooth module (BT-06, HC-05, HC-06) or ESP-01 as Wi-Fi bridge | `beginSerial(port, "Name")` |

- Wi-Fi is found automatically (mDNS); the app also connects by IP address.
- Up to 24 channels (8 on Uno/Nano/Mega because of their small RAM): voltage, current, temperature, humidity, pressure and [many more types](docs/PROTOCOL.md#5-channel-types)
- Power supply display with setpoints and CV/CC indicator, dual power supplies via groups
- Controls in the app: switch, button, slider, number input, selection list
- The app computes power and energy automatically from voltage and current
- Simple, open text protocol: [docs/PROTOCOL.md](docs/PROTOCOL.md)

## Installation

1. Arduino IDE: *Sketch > Include Library > Add .ZIP Library...* and select the ZIP of this repository (*Code > Download ZIP* on GitHub).
   Or copy the folder into `Documents/Arduino/libraries/MCUi`.
2. ESP32: board package (Espressif) version 3.x. ESP8266: board package "esp8266 by ESP8266 Community". All other boards: their usual board package.

**ESP32-S3, -C3, -C6 on core 2.x:** with *USB CDC On Boot: Enabled* the sketch stalls as long as no Serial Monitor is open, and Bluetooth stops working too. This is a known issue of core 2.x, fixed in 3.0.0. Update the core to 3.x, or set *Tools > USB CDC On Boot > Disabled*. Also set *Tools > Core Debug Level > None*.

## Quick start

```cpp
#include <MCUi.h>

MCUi mcui;

void setup() {
  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.begin("My Voltmeter");
}

void loop() {
  mcui.set("V", analogReadMilliVolts(4) / 1000.0 * 11);  // GPIO 4, divider 1:11
  mcui.update();
}
```

Open the MCU Instruments app, tap *Search*, select "My Voltmeter". Done.

## Examples

| Example | Content |
|---|---|
| `01_Voltmeter` | Voltage at an ADC pin through a voltage divider |
| `02_Labornetzteil` | Simulated lab power supply with CV/CC, output switch and setpoint sliders |
| `03_Temperatur_DS18B20` | Any number of DS18B20 sensors on one 1-Wire bus (up to 8) |
| `04_Bedienelemente` | Switch, slider, selection list and button control LEDs |
| `05_WLAN_Voltmeter` | Voltmeter over Wi-Fi |
| `06_Bluetooth_Classic` | Simulated signal over Bluetooth Classic (classic ESP32, Android) |
| `07_ESP01_WLAN_Bruecke` | Sketch for an ESP-01: turns it into a Wi-Fi module for any board |
| `08_Arduino_mit_Funkmodul` | Uno/Nano/Mega/STM32 voltmeter with BT-06 or ESP-01, incl. wiring |

## API overview

| Function | Purpose |
|---|---|
| `begin(name)` | Starts Bluetooth LE with the device name |
| `beginClassic(name)` | Starts Bluetooth Classic (classic ESP32 only) |
| `beginWiFi(name, ssid, password)` | Joins the Wi-Fi network and waits for the app |
| `beginWiFiAP(name, password)` | Opens its own Wi-Fi network (IP 192.168.4.1) |
| `beginSerial(port, name)` | Any serial radio module (BT-06, HC-05, ESP-01 bridge) |
| `setDebug(Serial)` | Status messages (connection, IP address) in the Serial Monitor |
| `addChannel(key, name, type, unit)` | Adds a measurement channel |
| `setRange`, `setDecimals`, `setGroup`, `setPage` | Display hints for a channel |
| `setSetpointOf(key, measuredKey)` | Channel is the setpoint of another channel |
| `setAlarm(key, low, high)` | Default alarm limits |
| `addSwitch`, `addButton`, `addSlider`, `addNumber`, `addSelect` | Controls in the app |
| `onControl(callback)` | Called when a control is changed in the app |
| `controlBool`, `controlFloat`, `controlText` | Current control values |
| `set(key, value)`, `setText(key, text)` | New values |
| `event(text)` | Shows a message in the app |
| `setOption(name, value)` | Display options, e.g. `setOption("cube", "1")` |
| `update()` | Call in every `loop()` |

**Important:** keys, names, units and option lists are stored as pointers. Use string literals (`"V"`) or global/static buffers, not temporary `String` objects.

## License

MIT, see [LICENSE](LICENSE).

---

## Deutsch

Arduino-Bibliothek für die App **MCU Instruments** (kurz MCUi): Messwerte vom Mikrocontroller auf Smartphone oder Tablet als Analoganzeige, Digitalanzeige, Netzteil-Ansicht oder Sensor-Kachel anzeigen und das Gerät aus der App bedienen.

**Installation:** In der Arduino IDE *Sketch > Bibliothek einbinden > .ZIP-Bibliothek hinzufügen...* und das ZIP dieses Repositories wählen. Benötigt wird das ESP32-Boardpaket von Espressif in Version 3.x.

**ESP32-S3, -C3, -C6 mit Boardpaket 2.x:** Bei *USB CDC On Boot: Enabled* bleibt der Sketch stehen, solange kein Serieller Monitor geöffnet ist, und Bluetooth funktioniert dann ebenfalls nicht. Das ist ein bekannter Fehler im Boardpaket 2.x, behoben ab 3.0.0. Abhilfe: Boardpaket auf 3.x aktualisieren oder *Werkzeuge > USB CDC On Boot > Disabled* wählen. Außerdem *Werkzeuge > Core Debug Level > Keine* einstellen.

**Erste Schritte:** Beispiel `01_Voltmeter` öffnen, hochladen, in der App auf *Suchen* tippen und das Gerät wählen.

**Verbindung:** `begin("Name")` für Bluetooth LE, `beginClassic("Name")` für Bluetooth Classic (nur klassischer ESP32, nur Android), `beginWiFi("Name", ssid, passwort)` für WLAN im vorhandenen Netz, `beginWiFiAP("Name", passwort)` für ein eigenes WLAN des ESP32 (IP 192.168.4.1, kein Router nötig). Beides auf allen ESP32 und dem ESP8266. Alle anderen Boards (Arduino Uno, Nano, Mega, STM32, RP2040 ...) nutzen `beginSerial(port, "Name")` mit einem Bluetooth-Modul (BT-06, HC-05) oder einem ESP-01 als WLAN-Brücke (Beispiele 07 und 08).

**Beispiele:** Voltmeter, Labornetzteil (simuliert, mit CV/CC), Temperaturen mit DS18B20, Bedienelemente (Schalter, Schieberegler, Auswahl, Taster), Voltmeter über WLAN, Bluetooth Classic, ESP-01 als WLAN-Brücke, Arduino mit Funkmodul.

**Wichtig:** Schlüssel, Namen, Einheiten und Optionslisten werden als Zeiger gespeichert. Bitte Zeichenketten-Literale (`"V"`) oder globale bzw. statische Puffer verwenden, keine temporären `String`-Objekte.

Das Protokoll ist offen dokumentiert in [docs/PROTOCOL.md](docs/PROTOCOL.md). Damit lassen sich auch andere Mikrocontroller oder eigene Firmware anbinden.
