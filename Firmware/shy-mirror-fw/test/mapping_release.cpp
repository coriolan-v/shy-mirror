#include "MappingRelease.h"
static_assert(mappingReleaseClear(true, true, 400, false, 350), "Far target releases");
static_assert(mappingReleaseClear(true, true, 350, false, 350), "Release boundary");
static_assert(!mappingReleaseClear(true, true, 349, false, 350), "Near target blocks");
static_assert(mappingReleaseClear(true, false, 0, true, 350), "Fresh weak return permits timed release");
static_assert(!mappingReleaseClear(false, false, 0, true, 350), "Stale/disconnected weak return blocks");
static_assert(!mappingReleaseClear(false, true, 1000, false, 350), "Bad transport blocks far data");
static_assert(!mappingReleaseClear(true, false, 1000, false, 350), "Other invalid statuses block");


static_assert(noTargetReading(true, true), "Healthy SignalFail is normal empty space");
static_assert(!noTargetReading(false, true), "Read errors/stale samples are not empty space");
static_assert(!noTargetReading(true, false), "Other range statuses are not empty space");
