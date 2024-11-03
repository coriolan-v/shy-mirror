#include <AccelStepper.h>

#define stepPin MISO
#define directionPin MOSI
#define enPin 5
#define HallSensor SCK

// 0-360 of the big cog
int calculatedPosition = 0;
int calculatedNumberOfRots = 0;

// Normally 0-3200 of the small cog
// 0-25600 of the large cog
int stepperPositionLib = 0;

// The number of sensors in your system.
const uint8_t numberOfSensors = 8;

int motorPosZones[numberOfSensors] = { 0, 400, 800, 1200, 1600, 2000, 2400, 2800 };

// Define a stepper and the pins it will use
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);

unsigned long prevMills_printMotorPos = 0;

void initMotor() {
  pinMode(HallSensor, INPUT_PULLUP);

  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH);
  // Change these to suit your stepper if you want

  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(2000);
  stepper.moveTo(10000);

  //homing();
}

void testMotor() {


  stepper.run();
}

void testZones() {
  for (int i = 0; i < numberOfSensors; i++) {
    Serial.print("Zone: "); Serial.println(i);
    stepper.runToNewPosition(motorPosZones[i]);
  }
}

// void loop()
// {
//   testMotor();
//     // If at the end of travel go to the other end
//     // if (stepper.distanceToGo() == 0)
//     //   stepper.moveTo(-stepper.currentPosition());


// }

void runMotor()
{
  stepper.run();

  stepperPositionLib = stepper.currentPosition()*8;

  if(millis() - prevMills_printMotorPos >= 100){
    prevMills_printMotorPos = millis();

    Serial.print("Stepper pos: "); Serial.print(stepper.currentPosition());
    Serial.print(", lib pos: "); Serial.print(stepperPositionLib);
    Serial.println();
  }
}

void motorHoming() {
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(2000);
  stepper.moveTo(3200 * 5);

  while (digitalRead(HallSensor) == HIGH) {
    stepper.run();
    Serial.println(stepper.currentPosition());
  }

  stepper.setCurrentPosition(0);
  stepperPositionLib = 0;
  Serial.println("Homing Complete");
}
