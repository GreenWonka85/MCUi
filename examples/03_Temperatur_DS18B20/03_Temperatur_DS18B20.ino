/*
  MCU Bench example 03: Temperatures with DS18B20

  Finds all DS18B20 sensors on one 1-Wire bus (up to 8) and shows each
  as its own channel. The app arranges them as tiles (phone: 4 per page,
  tablet: 8 per page).

  Wiring:
    DS18B20 DATA -> GPIO 4, with 4.7k pull-up to 3.3 V
    DS18B20 VDD  -> 3.3 V
    DS18B20 GND  -> GND

  Libraries (Arduino Library Manager):
    - OneWire
    - DallasTemperature

  Board: any ESP32 with Bluetooth.
*/
#include <MCUBench.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_PIN = 4;
const int MAX_SENSORS = 8;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
MCUBench bench;

// Keys and names must stay valid, so they live in static buffers
char keys[MAX_SENSORS][4];
char names[MAX_SENSORS][12];
DeviceAddress addresses[MAX_SENSORS];
int sensorCount = 0;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);

  sensorCount = sensors.getDeviceCount();
  if (sensorCount > MAX_SENSORS) sensorCount = MAX_SENSORS;
  Serial.printf("%d sensor(s) found\n", sensorCount);

  for (int i = 0; i < sensorCount; i++) {
    sensors.getAddress(addresses[i], i);
    snprintf(keys[i], sizeof(keys[i]), "T%d", i + 1);
    snprintf(names[i], sizeof(names[i]), "Sensor %d", i + 1);
    bench.addChannel(keys[i], names[i], "temperature");
    bench.setRange(keys[i], -20, 120);
    bench.setDecimals(keys[i], 1);
  }

  bench.setInterval(1000);  // temperatures change slowly
  bench.setFirmware("1.0");
  bench.begin("MCU Temperatur");
  sensors.requestTemperatures();
}

void loop() {
  static uint32_t lastRequest = 0;

  // A 12-bit conversion takes 750 ms
  if (millis() - lastRequest >= 1000) {
    for (int i = 0; i < sensorCount; i++) {
      float t = sensors.getTempC(addresses[i]);
      if (t != DEVICE_DISCONNECTED_C) bench.set(keys[i], t);
    }
    sensors.requestTemperatures();
    lastRequest = millis();
  }

  bench.update();
  delay(10);
}
