/*
  MCU Bench example 04: Controls (Bedienelemente)

  Shows all control types of the app:
    - Switch:  turns an LED on and off
    - Slider:  sets the brightness of a second LED (PWM)
    - Select:  blink mode of the first LED
    - Button:  resets a counter
  The device also sends values back (counter, brightness in percent).

  Wiring:
    LED 1 with 220 Ohm resistor at GPIO 2 (many boards have an LED there)
    LED 2 with 220 Ohm resistor at GPIO 5

  Board: any ESP32 with Bluetooth.
*/
#include <MCUBench.h>

const int LED_PIN = 2;
const int PWM_PIN = 5;

MCUBench bench;
uint32_t counter = 0;

void onControl(const char *key, const char *value) {
  Serial.printf("App changed %s = %s\n", key, value);

  if (strcmp(key, "RESET") == 0) {
    counter = 0;
    bench.event("Counter reset");
  }
  if (strcmp(key, "PWM") == 0) {
    analogWrite(PWM_PIN, (int)(atof(value) * 255 / 100));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  // Values from the device
  bench.addChannel("CNT", "Counter", "count");
  bench.setRange("CNT", 0, 1000);
  bench.addChannel("BRI", "Brightness", "percent");

  // Controls in the app
  bench.addSwitch("LED", "LED", false);
  bench.addSelect("BLINK", "Blink mode", "STEADY|SLOW|FAST", "STEADY");
  bench.addSlider("PWM", "Brightness", 0, 100, 1, 50, "%");
  bench.addButton("RESET", "Reset counter");
  bench.onControl(onControl);

  analogWrite(PWM_PIN, 50 * 255 / 100);

  bench.setFirmware("1.0");
  bench.begin("MCU Controls");
}

void loop() {
  static uint32_t lastCount = 0;
  if (millis() - lastCount >= 1000) {
    counter++;
    lastCount = millis();
  }

  // LED 1: on/off and blink mode
  bool ledOn = bench.controlBool("LED");
  const char *mode = bench.controlText("BLINK");
  uint32_t period = 0;
  if (strcmp(mode, "SLOW") == 0) period = 1000;
  if (strcmp(mode, "FAST") == 0) period = 200;
  bool blinkPhase = period == 0 || (millis() / (period / 2)) % 2 == 0;
  digitalWrite(LED_PIN, (ledOn && blinkPhase) ? HIGH : LOW);

  bench.set("CNT", counter);
  bench.set("BRI", bench.controlFloat("PWM"));

  bench.update();
  delay(10);
}
