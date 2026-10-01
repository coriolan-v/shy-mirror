// Compile with -std=c++14 -Iinclude -fsyntax-only.
#include "HallReference.h"
#include "MotionMath.h"

constexpr bool forwardCrossing() {
  HallReference hall;
  long edge = -1;
  hall.reset(false, 0);
  if (hall.update(true, 100, 15980, 1, edge)) return false;
  if (hall.update(true, 1099, 15986, 1, edge)) return false;
  if (!hall.update(true, 1100, 15987, 1, edge) || edge != 15980) return false;
  // Never continuously zero while the sensor remains active.
  if (hall.update(true, 3000, 16000, 1, edge)) return false;
  // Positive exit is the other side of the magnet, not the home boundary.
  hall.update(false, 4000, 16100, 1, edge);
  return !hall.update(false, 5000, 16106, 1, edge);
}
constexpr bool reverseCrossing() {
  HallReference hall;
  long edge = -1;
  hall.reset(false, 0);
  hall.update(true, 100, 100, -1, edge);
  if (hall.update(true, 1100, 94, -1, edge)) return false;
  hall.update(false, 2000, -20, -1, edge);
  return hall.update(false, 3000, -26, -1, edge) && edge == -20;
}
constexpr bool noiseAndReversal() {
  HallReference hall;
  long edge = -1;
  hall.reset(false, 0);
  hall.update(true, 100, 10, 1, edge);
  hall.update(false, 500, 12, 1, edge);
  if (hall.update(false, 1600, 18, 1, edge)) return false;
  hall.update(true, 2000, 20, 1, edge);
  return !hall.update(true, 3000, 19, -1, edge);
}
constexpr bool clockWrap() {
  HallReference hall;
  long edge = -1;
  hall.reset(false, 0xFFFFFF00u);
  hall.update(true, 0xFFFFFFF0u, 100, 1, edge);
  return hall.update(true, 984u, 106, 1, edge) && edge == 100;
}
static_assert(forwardCrossing(), "Forward entry: debounce, latched position, no repeated correction");
static_assert(reverseCrossing(), "Reverse exit uses the same physical boundary");
static_assert(noiseAndReversal(), "Reject short glitches and reversals within debounce");
static_assert(clockWrap(), "Debounce survives micros rollover");
static_assert(shortestStepDelta(15980, -6, 15999) == 13, "Preserve homing debounce offset");
static_assert(shortestStepDelta(-20, -6, 15999) == 14, "Reverse crossing correction");
static_assert(15987 + shortestStepDelta(15980, -6, 15999) == 16000,
              "Keep travel during debounce when applying correction");
