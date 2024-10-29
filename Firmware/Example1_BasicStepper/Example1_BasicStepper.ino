// Adafruit ItsyBity M4

#include "MKS_SERVO42.h"

byte const stepperId = 0;
uint8_t const speed = 20;
uint32_t const numberOfPulses = 50;

MKS_SERVO42 stepper;

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial2.begin(38400);
  //stepper.initialize(&Serial2);

  byte message[3];
	//byte checksum = stepperId + commandId;
	message[0] = 0xE0;
	message[1] = 0x80;
	message[2] = 0x10;

  Serial2.write(message, 3);

  //stepper.ping(stepperId);

  Serial.println("START");
}

byte direction = 1;

void loop() {
  // Serial.println(stepper.getCurrentPosition(stepperId));
  // direction = direction == 0 ? 1 : 0;
  // stepper.setTargetPosition(stepperId, direction, speed, numberOfPulses);
  // delay(5000);
}
