#include "Firmware.h"
#include "Config.h"
#include "MotionMath.h"
#include "InterruptStepper.h"
#include "HallReference.h"
#include <stdio.h>

#define stepPin MISO
#define directionPin MOSI
#define enPin 5
#define HallSensor SCK

// Preserve the working driver polarity.
const uint8_t motorEnableLevel = HIGH;
InterruptStepper stepper(stepPin, directionPin);
enum { IDLE, TEST_OUT, TEST_BACK, HOME_SEEK, AUTO_MOVE, HOME_RELEASE, CAL_RELEASE, CAL_SEEK, HOME_CHECK };
volatile uint8_t motorMode = IDLE;
uint32_t motorStartedAt = 0;
uint32_t lastHomingReportAt = 0;
uint32_t hallStateSince = 0;
int previousHall = HIGH;
long phaseStart = 0;
long testStart = 0;
long mirrorSteps = mirrorStepsPerRevolution;
bool homed = false;
bool calibrating = false;

volatile uint32_t motorHeartbeatUs = 0;
volatile bool motorServiceFault = false;
long automaticTargetAngle = -1;
uint32_t lastPersonSeenAt = 0;
bool inactivityReturnRequested = false;
HallReference hallReference;
long hallReferenceAngle = 0;
volatile long homingHallEdge = 0;
volatile bool homingHallEdgeKnown = false;

extern "C" void TC3_Handler() {
  if (!(TC3->COUNT16.INTFLAG.reg & TC_INTFLAG_MC0)) return;
  TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
  const uint8_t mode = motorMode;
  if (motorServiceFault) return;
  long edgePosition;
  if (hallReference.update(digitalRead(HallSensor) == LOW, micros(),
                           stepper.currentPosition(), stepper.motionDirection(), edgePosition)) {
    if (mode == HOME_SEEK || mode == CAL_SEEK) {
      homingHallEdge = edgePosition;
      homingHallEdgeKnown = true;
    } else if (mode == AUTO_MOVE && homed) {
      // Include steps taken during debounce; only correct the edge's error.
      stepper.correctPosition(shortestStepDelta(edgePosition, hallReferenceAngle, mirrorSteps));
    }
  }
  if (mode == IDLE || mode == HOME_CHECK) return;
  // Stop independently if foreground I2C/USB gets stuck. Main reports the fault.
  if (uint32_t(micros() - motorHeartbeatUs) > 250000UL) {
    motorServiceFault = true;
    digitalWrite(enPin, motorEnableLevel == LOW ? HIGH : LOW);
    return;
  }
  const bool constantSpeed = mode == HOME_SEEK || mode == HOME_RELEASE ||
    mode == CAL_RELEASE || mode == CAL_SEEK;
  stepper.service(constantSpeed);
}

void initStepTimer() {
  // Reserve TC3 for 40 kHz step servicing. GCLK0 follows the 120 MHz CPU clock.
  NVIC_DisableIRQ(TC3_IRQn);
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_TC3;
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = 0;
  while (GCLK->PCHCTRL[TC3_GCLK_ID].reg & GCLK_PCHCTRL_CHEN) {}
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TC3_GCLK_ID].reg & GCLK_PCHCTRL_CHEN)) {}
  TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC3->COUNT16.SYNCBUSY.reg || TC3->COUNT16.CTRLA.bit.SWRST) {}
  TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  while (TC3->COUNT16.SYNCBUSY.reg) {}
  TC3->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
  TC3->COUNT16.CC[0].reg = F_CPU / 40000UL - 1;
  while (TC3->COUNT16.SYNCBUSY.reg) {}
  TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
  TC3->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
  NVIC_SetPriority(TC3_IRQn, 2);
  NVIC_ClearPendingIRQ(TC3_IRQn);
  NVIC_EnableIRQ(TC3_IRQn);
  TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC3->COUNT16.SYNCBUSY.reg) {}
}



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
  motorHeartbeatUs = micros();
  hallReference.reset(readHallSensor() == LOW, micros());
  initStepTimer();
}

void stopMotor() {
  motorMode = IDLE;
  motorServiceFault = false;
  automaticTargetAngle = -1;
  homed = false; // Disabled coils or interrupted motion can lose the reference.
  calibrating = false;
  stepper.setCurrentPosition(stepper.currentPosition());
  digitalWrite(enPin, motorEnableLevel == LOW ? HIGH : LOW);
}

void testMotor() {
  stopMotor();
  testStart = stepper.currentPosition();
  motorStartedAt = millis();
  motorHeartbeatUs = micros();
  digitalWrite(enPin, motorEnableLevel);
  stepper.moveTo(testStart + 400);
  motorMode = TEST_OUT;
  Serial.println("Motor test: 400 pulses forward and back. Send home afterwards.");
}

void beginHoming(bool measureRevolution) {
  stopMotor();
  {
    StepperLock lock;
    homingHallEdgeKnown = false;
    hallReference.reset(readHallSensor() == LOW, micros());
  }
  calibrating = measureRevolution;
  motorStartedAt = millis();
  motorHeartbeatUs = micros();
  lastHomingReportAt = motorStartedAt;
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

// Shared accelerated motion for person avoidance and inactivity return.
bool moveAutomaticTarget(long target, long& current, long& delta, long& commandedTarget) {
  if (!homed || (motorMode != IDLE && motorMode != AUTO_MOVE)) return false;
  // Repeated observations must not reset acceleration or the movement deadline.
  if (motorMode == AUTO_MOVE && target == automaticTargetAngle) return false;
  {
    StepperLock lock;
    current = stepper.currentPosition();
    // Normalize only at rest. setCurrentPosition() would erase speed mid-move.
    if (motorMode == IDLE) {
      current = wrapSteps(current, mirrorSteps);
      stepper.setCurrentPosition(current);
      hallReference.reset(readHallSensor() == LOW, micros());
    }
    commandedTarget = equivalentTargetNear(current, target, mirrorSteps);
    delta = commandedTarget - current;
    if (motorMode == IDLE && delta == 0) return false;
    stepper.moveTo(commandedTarget); // Retarget with acceleration/deceleration preserved.
    automaticTargetAngle = target;
    motorStartedAt = millis();
    motorHeartbeatUs = micros();
    motorMode = AUTO_MOVE;
  }
  digitalWrite(enPin, motorEnableLevel);
  return true;
}

void updatePersonPresence(bool detected) {
  if (!homed) return; // Never restart a stopped, faulted, or unhomed motor.
  const uint32_t now = millis();
  if (detected) {
    lastPersonSeenAt = now;
    inactivityReturnRequested = false;
    return;
  }
  if (inactivityReturnRequested || uint32_t(now - lastPersonSeenAt) < returnHomeAfterMs) return;
  inactivityReturnRequested = true;
  long current, delta, target;
  // Zero is physical position 5.5. Keep sensing and Hall correction active.
  if (moveAutomaticTarget(0, current, delta, target) && DEBUG)
    Serial.println("No person for 60 seconds. Returning to home.");
}

void moveOppositePosition(uint8_t position, uint16_t distanceMm) {
  if (position < 1 || position > physicalZoneCount) return;
  // Always eight physical positions, even when position 8 is disabled.
  const long target = oppositePhysicalPositionTarget(position, mirrorSteps, motorPositionDirection);
  long current, delta, commandedTarget;
  if (!moveAutomaticTarget(target, current, delta, commandedTarget)) return;
  if (DEBUG) {
    const unsigned long now = millis();
    char timecode[24];
    snprintf(timecode, sizeof(timecode), "[%02lu:%02lu:%02lu.%03lu] ",
             now / 3600000UL, (now / 60000UL) % 60UL,
             (now / 1000UL) % 60UL, now % 1000UL);
    Serial.print(timecode);
    Serial.print("Person detected zone "); Serial.print(position);
    Serial.print(" | "); Serial.print(distanceMm); Serial.print(" mm");
    Serial.print(" | opposite zone "); Serial.print((position - 1 + physicalZoneCount / 2) % physicalZoneCount + 1);
    Serial.print(" | current position "); Serial.print(current); Serial.print(" pulses");
    Serial.print(" | target position "); Serial.print(commandedTarget); Serial.print(" pulses");
    Serial.print(" | moving opposite by "); Serial.print(delta); Serial.println(" pulses.");
  }
}

void completeHoming() {
  {
    StepperLock lock;
    motorMode = IDLE;
    // Homing stops after debounce, slightly inside the active Hall window.
    // Retain that offset so later fast crossings use exactly the same zero.
    // If booted on Hall, its entry boundary is the best available reference.
    hallReferenceAngle = homingHallEdgeKnown ? homingHallEdge - stepper.currentPosition() : 0;
    stepper.setCurrentPosition(0);
    automaticTargetAngle = -1;
    homed = true;
    lastPersonSeenAt = millis();
    inactivityReturnRequested = false;
    hallReference.reset(readHallSensor() == LOW, micros());
  }
  // Keep coils enabled so the reference remains held.
  Serial.println("Homing complete. Mirror at physical position 5.5 (between 5 and 6).");
  Serial.print("Geometry: 8 zones, 45 degrees each; pulses per mirror turn = "); Serial.println(mirrorSteps);
  Serial.println("Interactive mode active: mirror moves opposite the detected person.");
}

void runMotor() {
  motorHeartbeatUs = micros();
  if (motorServiceFault) {
    stopMotor();
    Serial.println("Motor paused: foreground stalled over 250 ms. Send home to resume.");
    return;
  }
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
  if (seeking && millis() - lastHomingReportAt >= 1000) {
    lastHomingReportAt = millis();
    Serial.print("Homing | position ");
    Serial.print(stepper.currentPosition());
    Serial.println(" pulses");
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
      const uint8_t completedMode = motorMode;
      motorMode = IDLE; // Freeze the Hall reference before logging or zeroing.
      if (completedMode == CAL_SEEK) {
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
        motorMode = IDLE;
        stepper.setCurrentPosition(0);
        homingHallEdgeKnown = false;
        phaseStart = 0;
        motorStartedAt = millis();
        motorHeartbeatUs = micros();
        stepper.setSpeed(homingSpeed);
        motorMode = CAL_RELEASE;
        resetHallDebounce();
        Serial.println("Home found. Measuring the next full revolution...");
        return;
      }
      completeHoming();
      return;
    }
    return; // TC3 services constant-speed steps.
  }
  if (stepper.isRunning()) return; // TC3 services accelerated steps.
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

