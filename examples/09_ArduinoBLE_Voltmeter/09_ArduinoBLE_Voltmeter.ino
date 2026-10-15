/*
  MCU Instruments example 09: Voltmeter on boards with built-in Bluetooth LE

  For Arduino boards that use the official ArduinoBLE library:
    - Arduino Nano 33 BLE / Nano 33 BLE Sense
    - Arduino Nano 33 IoT
    - Arduino UNO R4 WiFi
    - Arduino Nano RP2040 Connect
    - Portenta, Nicla, ... (everything ArduinoBLE supports)

  Install "ArduinoBLE" in the Library Manager. The #include <ArduinoBLE.h>
  BEFORE <MCUi.h> is required: it switches MCUi to ArduinoBLE.

  Works with Android and (later) iPhone, no pairing needed.

  Wiring: voltage divider at A0 as in example 01.
    Input + ---[ R1 100k ]---+---[ R2 10k ]--- GND
                             |
                             A0
  Nano 33 / RP2040 Connect: max. 3.3 V at A0. UNO R4: max. 5 V at A0.
*/
#include <ArduinoBLE.h>
#include <MCUi.h>

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
const float VREF = 5.0;
#else
const float VREF = 3.3;
#endif
const float ADC_MAX = 1023.0;  // analogRead() with 10 bits

const float R1 = 100000.0;     // without divider: R1 = 0
const float R2 = 10000.0;
const float DIVIDER = (R1 + R2) / R2;

MCUi mcui;

void setup() {
  Serial.begin(115200);

  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.setDecimals("V", 2);

  mcui.setDebug(Serial);
  mcui.setFirmware("1.0");
  if (!mcui.begin("MCUi BLE Voltmeter")) {
    Serial.println("Bluetooth LE could not be started.");
  }
}

void loop() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(A0);
  mcui.set("V", (sum / 16.0) / ADC_MAX * VREF * DIVIDER);

  mcui.update();
  delay(10);
}
