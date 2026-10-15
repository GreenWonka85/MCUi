/*
  MCU Instruments example 08: Any board with a radio module (Arduino Uno,
  Nano, Mega, STM32, RP2040, ...)

  The board sends its data over a serial port to a module:
    - Bluetooth LE:      HM-10, AT-09, HM-19 (Android and iPhone)
    - Bluetooth Classic: BT-06, HC-06, HC-05 (Android only)
    - Wi-Fi:             ESP-01 with the bridge sketch from example 07

  Wiring for Arduino Uno/Nano (SoftwareSerial on D2/D3):

    Module TXD  ->  D2
    Module RXD  <-  D3 through a voltage divider (module RX is 3.3 V):
                    D3 ---[ 1k ]---+---> module RXD
                                   |
                                 [ 2k ]
                                   |
                                  GND
    GND         ->  GND

    HM-10/AT-09, BT-06/HC-06: VCC to 5 V (most breakout boards have a regulator).
    ESP-01:      VCC and EN (CH_PD) to 3.3 V with at least 300 mA.
                 Do NOT use the 3.3 V pin of the Nano, it delivers too little.

  Boards with a second hardware port (Mega, STM32, RP2040, Leonardo ...) use
  Serial1 instead (TX1/RX1 pins), see below.

  Baud rate: 9600 (default of HM-10, AT-09, BT-06, HC-06; set the same in
  example 07).

  Let the app recognize the module: a name starting with "MCUi" is listed
  automatically. Set RENAME_BT_MODULE to 1 once to rename the module, then
  back to 0. Works with HM-10, AT-09, BT-06, HC-06 (HC-05 needs its AT mode,
  see its data sheet). HM-10 names have at most 12 characters.

  Measurement: voltage at A0 through a voltage divider, as in example 01.
*/
#include <MCUi.h>

#define RENAME_BT_MODULE 0
const char *DEVICE_NAME = "MCUi Volt";  // max. 12 characters for HM-10

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#include <SoftwareSerial.h>
SoftwareSerial module(2, 3);  // RX, TX
const float VREF = 5.0;
#else
auto &module = Serial1;  // second hardware serial port
#if defined(__AVR__)
const float VREF = 5.0;
#else
const float VREF = 3.3;
#endif
#endif

const float ADC_MAX = 1023.0;     // analogRead() with 10 bits
const float R1 = 100000.0;       // voltage divider, as in example 01
const float R2 = 10000.0;        // without divider: R1 = 0
const float DIVIDER = (R1 + R2) / R2;

MCUi mcui;

void setup() {
  module.begin(9600);

#if RENAME_BT_MODULE
  // Only works while no phone is connected
  delay(1000);
  module.print("AT+NAME");
  module.print(DEVICE_NAME);
  delay(1500);
#endif

  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.setDecimals("V", 2);

  mcui.setInterval(250);
  mcui.beginSerial(module, DEVICE_NAME);
}

void loop() {
  // Average 16 readings for a calm display
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(A0);
  float volts = (sum / 16.0) / ADC_MAX * VREF * DIVIDER;

  mcui.set("V", volts);
  mcui.update();
  delay(10);
}
