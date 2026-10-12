/*
  MCU Bench example 01: Voltmeter

  Measures a voltage at an analog pin through a voltage divider and shows
  it in the MCU Bench app as an analog gauge or digital display.

  Wiring (example for up to approx. 30 V):

    Input + ---[ R1 100k ]---+---[ R2 10k ]--- GND
                             |
                         GPIO 34 (ADC)

  The ESP32 ADC measures up to approx. 3.1 V. With 100k / 10k the
  divider factor is 11, so the maximum input is approx. 34 V.
  Never apply more than 3.3 V directly to the pin.

  Board: any ESP32 with Bluetooth (ESP32, ESP32-S3, ESP32-C3, ESP32-C6).
  For ESP32-C3/C6/S3 choose an ADC pin of your board (e.g. GPIO 2 or 4).
*/
#include <MCUBench.h>

const int ADC_PIN = 34;
const float R1 = 100000.0;  // Ohm, upper resistor
const float R2 = 10000.0;   // Ohm, lower resistor
const float DIVIDER = (R1 + R2) / R2;

MCUBench bench;

void setup() {
  Serial.begin(115200);

  bench.addChannel("V", "Voltage", "voltage");
  bench.setRange("V", 0, 30);
  bench.setDecimals("V", 2);

  bench.setFirmware("1.0");
  bench.begin("MCU Voltmeter");
  Serial.println("MCU Bench: waiting for the app ...");
}

void loop() {
  // Average 16 readings for a calm display
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(ADC_PIN);
  float volts = (sum / 16.0) / 1000.0 * DIVIDER;

  bench.set("V", volts);
  bench.update();
  delay(10);
}
