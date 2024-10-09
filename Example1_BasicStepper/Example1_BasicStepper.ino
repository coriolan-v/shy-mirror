// Adafruit ItsyBity M4

#include "MKS_SERVO42.h"

byte const stepperId = 0xe0;
uint8_t const speed = 20;
uint32_t const numberOfPulses = 50;

MKS_SERVO42 stepper;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial1.begin(38400);
  stepper.initialize(&Serial1);
  stepper.ping(stepperId);

  Serial.println("START");
}

byte direction = 1;

void loop() {
  Serial.println(stepper.getCurrentPosition(stepperId));
  direction = direction == 0 ? 1 : 0;
  stepper.setTargetPosition(stepperId, direction, speed, numberOfPulses);
  delay(5000);
}
