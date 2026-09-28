#pragma once
#include <stdint.h>

// DEBUG LOGGING: true = show detection details; false = hide them.
// Rebuild and upload after changing this flag.
constexpr bool DEBUG = true;

// Hardware IDs from 'sensors', not zone numbers.
// ID:      0   1   2   3   4   5   6   7
// XSHUT:  A0  A1  A2  A3   9  10  11  12
// Use 0 for all enabled; (1u << 5) | (1u << 6) disables IDs 5 and 6.
constexpr uint8_t disabledSensorMask = (1u << 0);
static_assert(disabledSensorMask != 0xFF, "At least one sensor must be enabled");

// Automatically start bounded, nonblocking homing after initialization.
constexpr bool homeOnStartup = true;

// Eight physical positions remain 45 degrees apart, including disabled positions.
// Initial estimate; run calibrate and replace with the measured value.
constexpr long mirrorStepsPerRevolution = 25600;
// +1 if positive motor rotation follows increasing mapped positions; otherwise -1.
constexpr int motorPositionDirection = 1;
constexpr long mirrorMoveSpeed = 800;
constexpr long mirrorAcceleration = 800;
constexpr long homingSpeed = 400;
static_assert(mirrorStepsPerRevolution >= 800 && mirrorStepsPerRevolution <= 64000,
              "Mirror revolution must be within calibration limits");
static_assert(motorPositionDirection == 1 || motorPositionDirection == -1,
              "Motor direction must be +1 or -1");
