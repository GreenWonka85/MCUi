/*
  MCU Instruments example 04: Controls (Bedienelemente)

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
#include <MCUi.h>

const int LED_PIN = 2;
const int PWM_PIN = 5;

MCUi mcui;
uint32_t counter = 0;

void onControl(const char *key, const char *value) {
  Serial.printf("App changed %s = %s\n", key, value);

  if (strcmp(key, "RESET") == 0) {
    counter = 0;
    mcui.event("Counter reset");
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
  mcui.addChannel("CNT", "Counter", "count");
  mcui.setRange("CNT", 0, 1000);
  mcui.addChannel("BRI", "Brightness", "percent");

  // Controls in the app
  mcui.addSwitch("LED", "LED", false);
  mcui.addSelect("BLINK", "Blink mode", "STEADY|SLOW|FAST", "STEADY");
  mcui.addSlider("PWM", "Brightness", 0, 100, 1, 50, "%");
  mcui.addButton("RESET", "Reset counter");
  mcui.onControl(onControl);

  analogWrite(PWM_PIN, 50 * 255 / 100);

  mcui.setDebug(Serial);  // status messages in the Serial Monitor
  mcui.setFirmware("1.0");
  mcui.begin("MCU Controls");
}

void loop() {
  static uint32_t lastCount = 0;
  if (millis() - lastCount >= 1000) {
    counter++;
    lastCount = millis();
  }

  // LED 1: on/off and blink mode
  bool ledOn = mcui.controlBool("LED");
  const char *mode = mcui.controlText("BLINK");
  uint32_t period = 0;
  if (strcmp(mode, "SLOW") == 0) period = 1000;
  if (strcmp(mode, "FAST") == 0) period = 200;
  bool blinkPhase = period == 0 || (millis() / (period / 2)) % 2 == 0;
  digitalWrite(LED_PIN, (ledOn && blinkPhase) ? HIGH : LOW);

  mcui.set("CNT", counter);
  mcui.set("BRI", mcui.controlFloat("PWM"));

  mcui.update();
  delay(10);
}
