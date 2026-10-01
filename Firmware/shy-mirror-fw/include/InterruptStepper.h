#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

// AccelStepper is not thread-safe. Keep foreground operations atomic relative
// to the timer ISR; never put I2C or Serial work inside this guard.
class StepperLock {
  uint32_t saved;
public:
  StepperLock() : saved(__get_PRIMASK()) { __disable_irq(); __DMB(); }
  ~StepperLock() { __DMB(); __set_PRIMASK(saved); }
  StepperLock(const StepperLock&) = delete;
  StepperLock& operator=(const StepperLock&) = delete;
};

class InterruptStepper : private AccelStepper {
  long coordinateOffset = 0;
public:
  InterruptStepper(uint8_t step, uint8_t dir) : AccelStepper(DRIVER, step, dir) {}
  void setMinPulseWidth(unsigned int value) { StepperLock lock; AccelStepper::setMinPulseWidth(value); }
  void setMaxSpeed(float value) { StepperLock lock; AccelStepper::setMaxSpeed(value); }
  void setAcceleration(float value) { StepperLock lock; AccelStepper::setAcceleration(value); }
  void setSpeed(float value) { StepperLock lock; AccelStepper::setSpeed(value); }
  void moveTo(long value) { StepperLock lock; AccelStepper::moveTo(value - coordinateOffset); }
  void setCurrentPosition(long value) { StepperLock lock; coordinateOffset = 0; AccelStepper::setCurrentPosition(value); }
  long currentPosition() { StepperLock lock; return AccelStepper::currentPosition() + coordinateOffset; }
  long targetPosition() { StepperLock lock; return AccelStepper::targetPosition() + coordinateOffset; }
  bool isRunning() { StepperLock lock; return AccelStepper::isRunning(); }
  int motionDirection() {
    StepperLock lock;
    const float velocity = AccelStepper::speed();
    return velocity > 0 ? 1 : (velocity < 0 ? -1 : 0);
  }
  // Rebase the reported position, keeping the physical target and ramp state.
  // Do not call setCurrentPosition here: it clears speed and acceleration.
  void correctPosition(long correction) {
    StepperLock lock;
    if (!correction) return;
    const long target = targetPosition();
    coordinateOffset += correction;
    AccelStepper::moveTo(target - coordinateOffset);
  }
  // ISR only: no logging, I2C or foreground calls.
  void service(bool constantSpeed) {
    if (constantSpeed) AccelStepper::runSpeed();
    else AccelStepper::run();
  }
};

