#include "Firmware.h"
#include "Config.h"
#include "MotionMath.h"
#include <AccelStepper.h>
#include <stdio.h>

#define stepPin MISO
#define directionPin MOSI
#define enPin 5
#define HallSensor SCK

// Preserve the working driver polarity.
const uint8_t motorEnableLevel = HIGH;
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);
enum { IDLE, TEST_OUT, TEST_BACK, HOME_SEEK, AUTO_MOVE, HOME_RELEASE, CAL_RELEASE, CAL_SEEK, HOME_CHECK };
uint8_t motorMode = IDLE;
uint32_t motorStartedAt = 0;
uint32_t hallStateSince = 0;
int previousHall = HIGH;
long phaseStart = 0;
long testStart = 0;
long mirrorSteps = mirrorStepsPerRevolution;
bool homed = false;
bool calibrating = false;

int readHallSensor() { return digitalRead(HallSensor); }
bool motorBusy() { return motorMode != IDLE; }
bool motorHomed() { return homed; }

void resetHallDebounce() {
  previousHall = readHallSensor();
  hallStateSince = millis();
}

bool hallStable(int level) {
  int state = readHallSensor();
  if (state != previousHall) {
    previousHall = state;
    hallStateSince = millis();
  }
  return state == level && millis() - hallStateSince >= 15;
}

void initMotor() {
  pinMode(HallSensor, INPUT_PULLUP);
  digitalWrite(enPin, motorEnableLevel == LOW ? HIGH : LOW);
  pinMode(enPin, OUTPUT);
  stepper.setMinPulseWidth(3);
  stepper.setMaxSpeed(mirrorMoveSpeed);
  stepper.setAcceleration(mirrorAcceleration);
}

void stopMotor() {
  motorMode = IDLE;
  homed = false; // Disabled coils or interrupted motion can lose the reference.
  calibrating = false;
  stepper.setCurrentPosition(stepper.currentPosition());
  digitalWrite(enPin, motorEnableLevel == LOW ? HIGH : LOW);
}

void testMotor() {
  stopMotor();
  testStart = stepper.currentPosition();
  motorStartedAt = millis();
  digitalWrite(enPin, motorEnableLevel);
  stepper.moveTo(testStart + 400);
  motorMode = TEST_OUT;
  Serial.println("Motor test: 400 pulses forward and back. Send home afterwards.");
}

void beginHoming(bool measureRevolution) {
  stopMotor();
  calibrating = measureRevolution;
  motorStartedAt = millis();
  phaseStart = stepper.currentPosition();
  digitalWrite(enPin, motorEnableLevel);
  stepper.setSpeed(homingSpeed);
  resetHallDebounce();
  if (readHallSensor() == LOW) {
    motorMode = measureRevolution ? HOME_RELEASE : HOME_CHECK;
    Serial.println(measureRevolution ?
      "Calibration: clearing Hall to establish a repeatable edge." :
      "Hall active: checking home without moving.");
  } else {
    motorMode = HOME_SEEK;
    Serial.println("Homing positive: seeking Hall.");
  }
  Serial.println("stop cancels. Maximum 180 seconds / 64000 pulses.");
}

void motorHoming() {
  beginHoming(false);
}

void calibrateMotor() {
  beginHoming(true);
  Serial.println("CALIBRATE: find home, then measure one full turn between Hall activations.");
  Serial.println("Requires one Hall activation per mirror revolution.");
  Serial.println("Observe whether this motion follows increasing physical positions.");
}

void moveOppositePosition(uint8_t position, uint16_t distanceMm) {
  if (!homed || motorBusy() || position < 1 || position > 8) return;
  long person = (long(position) - 1) * mirrorSteps / 8;
  long home = mirrorSteps * 9 / 16; // 202.5 degrees: halfway between positions 5/6.
  long target = oppositeTargetSteps(person, home, mirrorSteps, motorPositionDirection);
  long current = wrapSteps(stepper.currentPosition(), mirrorSteps);
  long delta = shortestStepDelta(current, target, mirrorSteps);
  if (delta == 0) return;
  stepper.setCurrentPosition(current); // Bound accumulated position without moving.
  stepper.moveTo(current + delta);
  motorStartedAt = millis();
  motorMode = AUTO_MOVE;
  digitalWrite(enPin, motorEnableLevel);
  if (DEBUG) {
    const unsigned long now = millis();
    char timecode[24];
    snprintf(timecode, sizeof(timecode), "[%02lu:%02lu:%02lu.%03lu] ",
             now / 3600000UL, (now / 60000UL) % 60UL,
             (now / 1000UL) % 60UL, now % 1000UL);
    Serial.print(timecode);
    Serial.print("Person detected zone "); Serial.print(position);
    Serial.print(" | "); Serial.print(distanceMm); Serial.print(" mm");
    Serial.print(" | moving opposite by "); Serial.print(delta); Serial.println(" pulses.");
  }
}

void completeHoming() {
  stepper.setCurrentPosition(0);
  motorMode = IDLE;
  homed = true;
  // Keep coils enabled so the reference remains held.
  Serial.println("Homing complete. Mirror at physical position 5.5 (between 5 and 6).");
  Serial.println("Automatic opposite movement enabled once a sensor map is saved.");
}

void runMotor() {
  if (!motorBusy()) return;
  bool seeking = motorMode == HOME_SEEK || motorMode == HOME_RELEASE ||
    motorMode == CAL_RELEASE || motorMode == CAL_SEEK || motorMode == HOME_CHECK;
  uint32_t limit = seeking ? 180000UL :
    (motorMode == AUTO_MOVE ? uint32_t(mirrorSteps * 1000UL / mirrorMoveSpeed + 5000) : 20000UL);
  if (millis() - motorStartedAt >= limit ||
      (seeking && stepper.currentPosition() - phaseStart >= 64000)) {
    stopMotor();
    Serial.println("Motor timeout/step limit. Automatic movement disabled; check Hall and driver, then home.");
    return;
  }
  if (motorMode == HOME_CHECK) {
    if (hallStable(LOW)) {
      Serial.println("Already on Hall sensor; homing movement skipped.");
      completeHoming();
    } else if (readHallSensor() == HIGH) {
      motorMode = HOME_SEEK;
      resetHallDebounce();
      Serial.println("Hall no longer active; seeking home.");
    }
    return; // No STEP pulses while confirming an already-active Hall sensor.
  }
  if (seeking) {
    if (motorMode == HOME_RELEASE || motorMode == CAL_RELEASE) {
      if (hallStable(HIGH)) {
        motorMode = motorMode == HOME_RELEASE ? HOME_SEEK : CAL_SEEK;
        resetHallDebounce();
      }
    } else if (hallStable(LOW)) {
      if (motorMode == CAL_SEEK) {
        long measured = stepper.currentPosition() - phaseStart;
        if (measured < 800 || measured > 64000) {
          stopMotor();
          Serial.println("Calibration rejected: implausible revolution count. Check Hall signal.");
          return;
        }
        mirrorSteps = measured;
        Serial.print("Measured mirrorStepsPerRevolution = "); Serial.println(mirrorSteps);
        Serial.println("Used now. Copy this number into Config.h to retain it after reset.");
        calibrating = false;
      } else if (calibrating) {
        stepper.setCurrentPosition(0);
        phaseStart = 0;
        motorStartedAt = millis();
        stepper.setSpeed(homingSpeed);
        motorMode = CAL_RELEASE;
        resetHallDebounce();
        Serial.println("Home found. Measuring the next full revolution...");
        return;
      }
      completeHoming();
      return;
    }
    stepper.runSpeed();
    return;
  }
  stepper.run();
  if (stepper.distanceToGo() != 0) return;
  if (motorMode == TEST_OUT) {
    stepper.moveTo(testStart);
    motorMode = TEST_BACK;
  } else if (motorMode == TEST_BACK) {
    stopMotor();
    Serial.println("Motor test complete. Send home to enable automatic behavior.");
  } else {
    motorMode = IDLE; // Hold position; retain the homing reference.
  }
}

