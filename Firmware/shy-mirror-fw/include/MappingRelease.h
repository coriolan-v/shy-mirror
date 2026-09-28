#pragma once
#include <stdint.h>

// SignalFail is accepted as no usable target, not a measured distance.
constexpr bool noTargetReading(bool healthyFresh, bool signalFail) {
  return healthyFresh && signalFail;
}

// Only a healthy, fresh weak-return measurement may substitute for a far range.
// Hardware/I2C failures and stale data must never count as hand removal.
constexpr bool mappingReleaseClear(bool healthyFresh, bool valid, uint16_t distanceMm,
                                   bool weakReturn, uint16_t releaseMm) {
  return healthyFresh && ((valid && distanceMm >= releaseMm) || (!valid && weakReturn));
}

