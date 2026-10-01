#pragma once
#include <stdint.h>

// C++14 permits evaluating the same state machine in compile-time regression tests.
#if __cplusplus >= 201402L
#define HALL_CONSTEXPR constexpr
#else
#define HALL_CONSTEXPR
#endif

// Capture the edge position immediately, then reject pulses shorter than 1 ms.
// Positive entry and negative exit refer to the SAME side of the Hall window.
class HallReference {
  bool rawActive = false;
  bool stableActive = false;
  uint32_t changedAt = 0;
  long edgePosition = 0;
  int edgeDirection = 0;
public:
  HALL_CONSTEXPR void reset(bool active, uint32_t now) {
    rawActive = stableActive = active;
    changedAt = now;
    edgeDirection = 0;
  }
  HALL_CONSTEXPR bool update(bool active, uint32_t now, long position, int direction,
              long& referencePosition) {
    if (active != rawActive) {
      rawActive = active;
      changedAt = now;
      edgePosition = position;
      edgeDirection = direction;
    }
    if (rawActive == stableActive || uint32_t(now - changedAt) < 1000) return false;
    stableActive = rawActive;
    if (direction != edgeDirection) return false;
    if ((rawActive && edgeDirection == 1) || (!rawActive && edgeDirection == -1)) {
      referencePosition = edgePosition;
      return true;
    }
    return false;
  }
};

#undef HALL_CONSTEXPR
