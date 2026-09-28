#pragma once

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

