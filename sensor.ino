#include <Wire.h>
#include <string.h>
#include "MS5837_02BA.hpp"
MS5837_02BA sensor;
void setup() {
  Serial.begin(9600);
  Wire.begin();
  sensor = MS5837_02BA(Wire);
  sensor.update();
}
void loop() {
  char sending_buffer[4] = { 0 };
  float data_buffer = 0;
  sensor.update();
  data_buffer = sensor.get_temperature();
  memcpy(sending_buffer, &data_buffer, 4);
  Serial.write(sending_buffer);

  delay(10);

  data_buffer = sensor.get_pressure();
  memcpy(sending_buffer, &data_buffer, 4);
  Serial.write(sending_buffer);

  delay(1000);
}
