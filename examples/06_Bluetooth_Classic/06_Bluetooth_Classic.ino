/*
  MCU Instruments example 06: Bluetooth Classic

  Sends a simulated signal over Bluetooth Classic (Serial Port Profile).
  Runs without extra hardware.

  Requirements:
    - Classic ESP32 (e.g. ESP32-WROOM-32, ESP32 DevKit V1).
      ESP32-S2, -S3, -C3 and -C6 have no Bluetooth Classic, use example 01
      (Bluetooth LE) or 05 (Wi-Fi) there.
    - Android. iPhones do not allow Bluetooth Classic for apps.

  First pair the ESP32 once in the Android Bluetooth settings
  ("MCU Classic"). It then appears in the app under "Bluetooth (paired)".
*/
#include <MCUi.h>

MCUi mcui;

void setup() {
  Serial.begin(115200);

  mcui.addChannel("U", "Signal", "voltage");
  mcui.setRange("U", -5, 5);
  mcui.setDecimals("U", 2);

  mcui.addChannel("F", "Frequency", "frequency");
  mcui.setRange("F", 0, 2);

  mcui.addSlider("FSET", "Frequency", 0.1, 2.0, 0.1, 0.5, "Hz");

  mcui.setDebug(Serial);
  mcui.setFirmware("1.0");
  if (!mcui.beginClassic("MCU Classic")) {
    Serial.println("This board has no Bluetooth Classic. Please use a classic ESP32.");
  }
}

void loop() {
  float f = mcui.controlFloat("FSET");
  float t = millis() / 1000.0;
  mcui.set("U", 4.0 * sin(2 * PI * f * t));
  mcui.set("F", f);
  mcui.update();
  delay(10);
}
