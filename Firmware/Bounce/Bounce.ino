// Bounce.pde
// -*- mode: C++ -*-
//
// Make a single stepper bounce from one limit to another
//
// Copyright (C) 2012 Mike McCauley
// $Id: Random.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

#include <AccelStepper.h>

// Define a stepper and the pins it will use
#define stepPin 11
#define directionPin 10
#define enPin 12
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);

void setup()
{  
  // Change these to suit your stepper if you want
  stepper.setMaxSpeed(24000);
  stepper.setAcceleration(24000);
  stepper.moveTo(60000);
}

void loop()
{
    // If at the end of travel go to the other end
    if (stepper.distanceToGo() == 0)
      stepper.moveTo(-stepper.currentPosition());

    stepper.run();
}
