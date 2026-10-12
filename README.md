# MCUBench

Arduino library for the **MCU Bench** app: show measurements from your microcontroller on your phone or tablet as analog gauges, digital displays, power supply panels and sensor tiles, and control your device from the app.

*Deutsch: siehe unten.*

## Features (version 0.1)

- Bluetooth LE on ESP32, ESP32-S3, ESP32-C3 and ESP32-C6 (Nordic UART Service)
- Up to 24 channels: voltage, current, temperature, humidity, pressure and [many more types](docs/PROTOCOL.md#5-channel-types)
- Power supply display with setpoints and CV/CC indicator, dual power supplies via groups
- Controls in the app: switch, button, slider, number input, selection list
- The app computes power and energy automatically from voltage and current
- Simple, open text protocol: [docs/PROTOCOL.md](docs/PROTOCOL.md)

Bluetooth Classic, Wi-Fi and further boards follow in later versions.

## Installation

1. Arduino IDE: *Sketch > Include Library > Add .ZIP Library...* and select the ZIP of this repository (*Code > Download ZIP* on GitHub).
   Or copy the folder into `Documents/Arduino/libraries/MCUBench`.
2. ESP32 board package (Espressif) version 2.0.11 or newer, 3.x recommended.

## Quick start

```cpp
#include <MCUBench.h>

MCUBench bench;

void setup() {
  bench.addChannel("V", "Voltage", "voltage");
  bench.setRange("V", 0, 30);
  bench.begin("My Voltmeter");
}

void loop() {
  bench.set("V", analogReadMilliVolts(34) / 1000.0 * 11);
  bench.update();
}
```

Open the MCU Bench app, tap *Search*, select "My Voltmeter". Done.

## Examples

| Example | Content |
|---|---|
| `01_Voltmeter` | Voltage at an ADC pin through a voltage divider |
| `02_Labornetzteil` | Simulated lab power supply with CV/CC, output switch and setpoint sliders |
| `03_Temperatur_DS18B20` | Any number of DS18B20 sensors on one 1-Wire bus (up to 8) |
| `04_Bedienelemente` | Switch, slider, selection list and button control LEDs |

## API overview

| Function | Purpose |
|---|---|
| `begin(name)` | Starts Bluetooth LE with the device name |
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

Arduino-Bibliothek für die App **MCU Bench**: Messwerte vom Mikrocontroller auf Smartphone oder Tablet als Analoganzeige, Digitalanzeige, Netzteil-Ansicht oder Sensor-Kachel anzeigen und das Gerät aus der App bedienen.

**Installation:** In der Arduino IDE *Sketch > Bibliothek einbinden > .ZIP-Bibliothek hinzufügen...* und das ZIP dieses Repositories wählen. Benötigt wird das ESP32-Boardpaket von Espressif ab Version 2.0.11, empfohlen 3.x.

**Erste Schritte:** Beispiel `01_Voltmeter` öffnen, hochladen, in der App auf *Suchen* tippen und das Gerät wählen.

**Beispiele:** Voltmeter, Labornetzteil (simuliert, mit CV/CC), Temperaturen mit DS18B20, Bedienelemente (Schalter, Schieberegler, Auswahl, Taster).

**Wichtig:** Schlüssel, Namen, Einheiten und Optionslisten werden als Zeiger gespeichert. Bitte Zeichenketten-Literale (`"V"`) oder globale bzw. statische Puffer verwenden, keine temporären `String`-Objekte.

Das Protokoll ist offen dokumentiert in [docs/PROTOCOL.md](docs/PROTOCOL.md). Damit lassen sich auch andere Mikrocontroller oder eigene Firmware anbinden.
