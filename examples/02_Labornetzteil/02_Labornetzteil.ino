/*
  MCU Instruments example 02: Lab power supply (Labornetzteil)

  Shows a power supply in the app: voltage and current with setpoint
  marks, CV/CC indicator, output switch and sliders for the setpoints.
  The app computes power and energy automatically.

  This sketch simulates the power supply with a load resistor, so it runs
  without extra hardware. For a real power supply replace the marked parts
  with your ADC readings and your DAC/PWM output.

  Board: any ESP32 with Bluetooth.
*/
#include <MCUi.h>

MCUi mcui;

// Simulated load in Ohm
const float LOAD_OHM = 8.0;

void onControl(const char *key, const char *value) {
  // Called when a control is changed in the app.
  // Here you would set your DAC/PWM for voltage and current limit.
  Serial.printf("App changed %s = %s\n", key, value);
}

void setup() {
  Serial.begin(115200);

  // Measured values
  mcui.addChannel("V", "Voltage", "voltage");
  mcui.setRange("V", 0, 30);
  mcui.setDecimals("V", 2);

  mcui.addChannel("I", "Current", "current");
  mcui.setRange("I", 0, 5);
  mcui.setDecimals("I", 3);

  mcui.addChannel("MODE", "Mode", "psu_mode");

  // Setpoints as marks on the gauges
  mcui.addChannel("VS", "Voltage set", "voltage");
  mcui.setRange("VS", 0, 30);
  mcui.setSetpointOf("VS", "V");

  mcui.addChannel("IS", "Current limit", "current");
  mcui.setRange("IS", 0, 5);
  mcui.setSetpointOf("IS", "I");

  // Controls
  mcui.addSwitch("OUT", "Output", false);
  mcui.addSlider("VSET", "Voltage", 0, 30, 0.1, 12.0, "V");
  mcui.addSlider("ISET", "Current limit", 0, 5, 0.01, 1.0, "A");
  mcui.onControl(onControl);

  mcui.setDebug(Serial);  // status messages in the Serial Monitor
  mcui.setFirmware("1.0");
  mcui.begin("MCU Netzteil");
}

void loop() {
  bool on = mcui.controlBool("OUT");
  float vSet = mcui.controlFloat("VSET");
  float iSet = mcui.controlFloat("ISET");

  // ---- Simulation (replace with real measurements) ----
  float volts = 0, amps = 0;
  const char *mode = "OFF";
  if (on) {
    float iWanted = vSet / LOAD_OHM;
    if (iWanted <= iSet) {
      volts = vSet;            // constant voltage
      amps = iWanted;
      mode = "CV";
    } else {
      amps = iSet;             // current limit reached
      volts = iSet * LOAD_OHM;
      mode = "CC";
    }
    volts += random(-5, 6) / 1000.0;  // a little noise
    amps += random(-2, 3) / 10000.0;
    if (amps < 0) amps = 0;
  }
  // -----------------------------------------------------

  mcui.set("V", volts);
  mcui.set("I", amps);
  mcui.setText("MODE", mode);
  mcui.set("VS", vSet);
  mcui.set("IS", iSet);

  mcui.update();
  delay(10);
}
