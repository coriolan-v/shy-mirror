#pragma once
#include <stdint.h>
#include "MotionMath.h"

// DEBUG LOGGING: true = show detection details; false = hide them.
// Rebuild and upload after changing this flag.
constexpr bool DEBUG = true;

// Hardware IDs from 'sensors', not zone numbers.
// ID:      0   1   2   3   4   5   6   7
// XSHUT:  A0  A1  A2  A3   9  10  11  12
// Use 0 for all enabled; (1u << 5) | (1u << 6) disables IDs 5 and 6.
constexpr uint8_t disabledSensorMask = (1u << 5);
static_assert(disabledSensorMask != 0xFF, "At least one sensor must be enabled");

// Fixed physical positions 1-8 -> hardware IDs above.
// Position:  1   2   3   4   5   6   7   8
// XSHUT:     9  A3  A2  A1  A0  12  11  10
// Pin 10 is physical position 8; its existing disabled bit remains in effect.
constexpr uint8_t sensorHardwareIdByPosition[physicalZoneCount] = {4, 3, 2, 1, 0, 7, 6, 5};

// Automatically start bounded, nonblocking homing after initialization.
constexpr bool homeOnStartup = true;
// Return to the established home position after one minute without a detection.
constexpr uint32_t returnHomeAfterMs = 60000;

// Eight physical positions remain 45 degrees apart, including disabled positions.
// Measured with the Hall sensor: 15,999 STEP pulses per mirror revolution.
constexpr long mirrorStepsPerRevolution = 15999;
// +1 if positive motor rotation follows increasing physical positions; otherwise -1.
constexpr int motorPositionDirection = 1;
constexpr long mirrorMoveSpeed = 2500;
constexpr long mirrorAcceleration = 3200;
constexpr long homingSpeed = 1300;
static_assert(mirrorStepsPerRevolution >= 800 && mirrorStepsPerRevolution <= 64000,
              "Mirror revolution must be within calibration limits");
static_assert(motorPositionDirection == 1 || motorPositionDirection == -1,
              "Motor direction must be +1 or -1");

// Fast tracking settings required by the sensor loop.
constexpr uint32_t sensorTimingBudgetUs = 20000;
constexpr uint32_t sensorPeriodMs = 30;
constexpr uint32_t personConfirmMs = 100;
constexpr uint32_t sensorStaleMs = 100;
constexpr int sensorSmoothingDivisor = 2;
static_assert(mirrorMoveSpeed > 0 && mirrorMoveSpeed <= 10000, "Speed must fit 40 kHz step service");
static_assert(mirrorAcceleration > 0 && homingSpeed > 0 && homingSpeed <= 10000, "Invalid motor rates");
static_assert(sensorTimingBudgetUs >= 20000 && sensorPeriodMs * 1000 > sensorTimingBudgetUs,
              "Sensor period must exceed Short-mode timing budget");
static_assert(sensorSmoothingDivisor >= 1, "Smoothing divisor must be positive");
