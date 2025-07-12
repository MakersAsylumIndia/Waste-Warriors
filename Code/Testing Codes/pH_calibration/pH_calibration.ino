#include "DFRobot_PH.h"
#include <EEPROM.h>

#define PH_PIN 35  // Use ADC-capable pin: 32, 33, 34, 35 on ESP32

float voltage, phValue, temperature = 25;
DFRobot_PH ph;  // ✅ Declare the ph object globally

void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);  // ✅ Needed for ESP32
  ph.begin();
}

void loop() {
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) {
    timepoint = millis();

    voltage = analogRead(PH_PIN) / 4095.0 * 3300;  // Use correct ADC range for ESP32

    phValue = ph.readPH(voltage, temperature);
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print(" mV\t");
    Serial.print("pH: ");
    Serial.println(phValue, 2);
  }

  ph.calibration(voltage, temperature);  // Serial-based calibration
}