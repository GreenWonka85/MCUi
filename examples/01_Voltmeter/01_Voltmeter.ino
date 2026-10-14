/*
  MCU Instruments example 01: Voltmeter

  Measures a voltage at an analog pin through a voltage divider and shows
  it in the MCU Instruments app as an analog gauge or digital display.

  Wiring (example for up to approx. 30 V):

    Input + ---[ R1 100k ]---+---[ R2 10k ]--- GND
                             |
                          ADC pin

  The ESP32 ADC measures up to approx. 3.1 V. With 100k / 10k the
  divider factor is 11, so the maximum input is approx. 34 V.
  Never apply more than 3.3 V directly to the pin.

  Board: any ESP32 with Bluetooth. The ADC pin is chosen automatically:
    ESP32     GPIO 34
    ESP32-S3  GPIO 4
    ESP32-C3  GPIO 3
    ESP32-C6  GPIO 3
  You can change ADC_PIN below to any ADC1 pin of your board.
*/
#include <MCUi.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
const int ADC_PIN = 4;
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
const int ADC_PIN = 3;
#else
const int ADC_PIN = 34;
#endif
// Voltage divider. The measured pin voltage is multiplied by (R1 + R2) / R2.
// Without a divider (direct measurement, max. 3.1 V) set R1 = 0.
const float R1 = 100000.0;  // Ohm, upper resistor
const float R2 = 10000.0;   // Ohm, lower resistor
const float DIVIDER = (R1 + R2) / R2;

MCUi mcui;

void setup() {
  Serial.begin(115200);

  if (digitalPinToAnalogChannel(ADC_PIN) < 0) {
    Serial.printf("ADC_PIN %d is not an analog input on this board. Please change ADC_PIN.\n", ADC_PIN);
  }

  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.setDecimals("V", 2);

  mcui.setDebug(Serial);  // status messages in the Serial Monitor
  mcui.setFirmware("1.0");
  mcui.begin("MCU Voltmeter");
  Serial.println("MCUi: waiting for the app ...");
}

void loop() {
  // A wrong pin would flood the serial port with error messages, so skip it
  if (digitalPinToAnalogChannel(ADC_PIN) >= 0) {
    // Average 16 readings for a calm display
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(ADC_PIN);
    float volts = (sum / 16.0) / 1000.0 * DIVIDER;
    mcui.set("V", volts);
  }
  mcui.update();
  delay(10);
}
