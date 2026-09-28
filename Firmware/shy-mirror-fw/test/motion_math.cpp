// Compile-only assertions; run with arm-none-eabi-g++ -std=c++11 -Iinclude -fsyntax-only test/motion_math.cpp
#include "MotionMath.h"
static_assert(wrapSteps(-1, 25600) == 25599, "Negative positions wrap");
static_assert(wrapSteps(25600, 25600) == 0, "Full turn wraps");
static_assert(shortestStepDelta(25000, 100, 25600) == 700, "Cross zero forward");
static_assert(shortestStepDelta(100, 25000, 25600) == -700, "Cross zero backward");
static_assert(shortestStepDelta(0, 12800, 25600) == -12800, "Half turn tie");
static_assert(shortestStepDelta(400, 400, 25600) == 0, "No redundant movement");
// Eight 45-degree positions: home at 202.5 degrees, halfway between positions 5/6.
static_assert(oppositeTargetSteps(0, 14400, 25600, 1) == 24000, "Person at position 1");
static_assert(oppositeTargetSteps(12800, 14400, 25600, 1) == 11200, "Person at position 5");
static_assert(oppositeTargetSteps(16000, 14400, 25600, 1) == 14400, "Person at position 6");
static_assert(oppositeTargetSteps(0, 14400, 25600, -1) == 1600, "Reversed motor direction");
static_assert(oppositeTargetSteps(14400, 14400, 25600, 1) == 12800, "Person in home direction");

