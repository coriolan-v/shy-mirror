#pragma once

// Physical geometry never depends on the number of enabled sensors.
constexpr unsigned physicalZoneCount = 8;

// Normalize to [0, revolution). Inputs and outputs are STEP pulses.
constexpr long wrapSteps(long value, long revolution) {
  return ((value % revolution) + revolution) % revolution;
}

// Exactly half a turn consistently takes the negative direction.
constexpr long shortestStepDelta(long current, long target, long revolution) {
  return wrapSteps(target - current + revolution / 2, revolution) - revolution / 2;
}

constexpr long oppositeTargetSteps(long personSteps, long homeSteps, long revolution, int direction) {
  return wrapSteps(direction * (personSteps + revolution / 2 - homeSteps), revolution);
}


// Choose the nearest equivalent angle without resetting an in-flight coordinate.
constexpr long equivalentTargetNear(long current, long angle, long revolution) {
  return current + shortestStepDelta(wrapSteps(current, revolution), angle, revolution);
}

constexpr long oppositePhysicalPositionTarget(unsigned position, long revolution, int direction) {
  return oppositeTargetSteps((long(position) - 1) * revolution / physicalZoneCount,
                             revolution * 9 / 16, revolution, direction);
}
