/*
  MCU Bench example 02: Lab power supply (Labornetzteil)

  Shows a power supply in the app: voltage and current with setpoint
  marks, CV/CC indicator, output switch and sliders for the setpoints.
  The app computes power and energy automatically.

  This sketch simulates the power supply with a load resistor, so it runs
  without extra hardware. For a real power supply replace the marked parts
  with your ADC readings and your DAC/PWM output.

  Board: any ESP32 with Bluetooth.
*/
#include <MCUBench.h>

MCUBench bench;

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
  bench.addChannel("V", "Voltage", "voltage");
  bench.setRange("V", 0, 30);
  bench.setDecimals("V", 2);

  bench.addChannel("I", "Current", "current");
  bench.setRange("I", 0, 5);
  bench.setDecimals("I", 3);

  bench.addChannel("MODE", "Mode", "psu_mode");

  // Setpoints as marks on the gauges
  bench.addChannel("VS", "Voltage set", "voltage");
  bench.setRange("VS", 0, 30);
  bench.setSetpointOf("VS", "V");

  bench.addChannel("IS", "Current limit", "current");
  bench.setRange("IS", 0, 5);
  bench.setSetpointOf("IS", "I");

  // Controls
  bench.addSwitch("OUT", "Output", false);
  bench.addSlider("VSET", "Voltage", 0, 30, 0.1, 12.0, "V");
  bench.addSlider("ISET", "Current limit", 0, 5, 0.01, 1.0, "A");
  bench.onControl(onControl);

  bench.setFirmware("1.0");
  bench.begin("MCU Netzteil");
}

void loop() {
  bool on = bench.controlBool("OUT");
  float vSet = bench.controlFloat("VSET");
  float iSet = bench.controlFloat("ISET");

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

  bench.set("V", volts);
  bench.set("I", amps);
  bench.setText("MODE", mode);
  bench.set("VS", vSet);
  bench.set("IS", iSet);

  bench.update();
  delay(10);
}
