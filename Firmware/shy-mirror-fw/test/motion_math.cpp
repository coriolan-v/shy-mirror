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


static_assert(equivalentTargetNear(25000, 100, 25600) == 25700, "Retarget forward across zero");
static_assert(equivalentTargetNear(100, 25000, 25600) == -600, "Retarget backward across zero");
static_assert(equivalentTargetNear(-500, 100, 25600) == 100, "Retarget from negative coordinate");
static_assert(equivalentTargetNear(26000, 25000, 25600) == 25000, "Preserve accumulated coordinate");
static_assert(equivalentTargetNear(25700, 100, 25600) == 25700, "Already at equivalent target");

static_assert(physicalZoneCount == 8, "Disabled sensors must not compress the circle");
static_assert(oppositePhysicalPositionTarget(1, 25600, 1) == 24000, "Zone 1 -> zone 5");
static_assert(oppositePhysicalPositionTarget(2, 25600, 1) == 1600, "Zone 2 -> zone 6");
static_assert(oppositePhysicalPositionTarget(3, 25600, 1) == 4800, "Zone 3 -> zone 7");
static_assert(oppositePhysicalPositionTarget(4, 25600, 1) == 8000, "Zone 4 -> zone 8");
static_assert(oppositePhysicalPositionTarget(5, 25600, 1) == 11200, "Zone 5 -> zone 1");
static_assert(oppositePhysicalPositionTarget(6, 25600, 1) == 14400, "Zone 6 -> zone 2");
static_assert(oppositePhysicalPositionTarget(7, 25600, 1) == 17600, "Zone 7 -> zone 3");
static_assert(oppositePhysicalPositionTarget(8, 25600, 1) == 20800, "Zone 8 -> zone 4");
static_assert(oppositePhysicalPositionTarget(4, 3200, 1) == 1000, "Different gearing scales all zones");

// Measured odd revolution count: rounding stays within one STEP pulse.
static_assert(oppositePhysicalPositionTarget(1, 15999, 1) == 14999, "Measured zone 1");
static_assert(oppositePhysicalPositionTarget(2, 15999, 1) == 999, "Measured zone 2");
static_assert(oppositePhysicalPositionTarget(3, 15999, 1) == 2999, "Measured zone 3");
static_assert(oppositePhysicalPositionTarget(4, 15999, 1) == 4999, "Measured zone 4");
static_assert(oppositePhysicalPositionTarget(5, 15999, 1) == 6999, "Measured zone 5");
static_assert(oppositePhysicalPositionTarget(6, 15999, 1) == 8999, "Measured zone 6");
static_assert(oppositePhysicalPositionTarget(7, 15999, 1) == 10999, "Measured zone 7");
static_assert(oppositePhysicalPositionTarget(8, 15999, 1) == 12999, "Measured zone 8");
static_assert(shortestStepDelta(0, 14999, 15999) == -1000, "Measured wraparound");
