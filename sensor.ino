#include <Wire.h>
#include "MS5837_02BA.hpp"
MS5837_02BA sensor;
void setup() {
  Serial.begin(9600);
  Wire.begin();
  sensor = MS5837_02BA(Wire);
  sensor.update();
  Serial.print("Current temp:");
  Serial.println(sensor.get_temperature());
  Serial.print("Current pressure:");
  Serial.println(sensor.get_pressure());
}
void loop() {
  // sensor.update();
  // Serial.print("Current temp:");
  // Serial.println(sensor.get_temperature());
  // Serial.print("Current pressure:");
  // Serial.println(sensor.get_pressure());
  // delay(1000);
}