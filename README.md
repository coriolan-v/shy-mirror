# Shy mirror firmware

Target: Adafruit ItsyBitsy M4 Express (SAMD51G19A). Open this folder in VS Code
with PlatformIO. Dependencies are pinned in platformio.ini.

```powershell
pio run
pio run -t upload
pio device monitor
```

The serial monitor uses 115200 baud, local echo, and send-on-enter.
Close it before uploading. Firmware upload erases the saved sensor map.

## First setup: measure motion and map physical positions

1. Upload and let startup homing finish, or send stop to cancel it.
2. Send calibrate. The mirror seeks a Hall reference, then measures a full
   revolution between two Hall activations. There must be exactly one magnet /
   active Hall region per mirror revolution.
3. Note the printed mirrorStepsPerRevolution value. It is used immediately,
   but copy it into include/Config.h to retain it after reset. The initial
   25,600 value is an estimate, not a confirmed gear ratio.
4. During calibration, observe which way the mirror turns. Set
   motorPositionDirection to +1 if this follows increasing physical positions
   (1, 2, 3, ... 8), or -1 if it goes the other way. This cannot be inferred
   electrically from STEP/DIR and one Hall sensor.
5. Rebuild/upload after editing Config.h, then map as described below.
6. Send home after mapping. Successful homing enables automatic behavior.

Startup homing and the home command accept Hall already LOW after a 15 ms
confirmation, set the current position to zero and skip homing movement. If Hall
is HIGH, they seek home in the positive direction. Automatic behavior can move
the mirror afterwards if a mapped sensor detects a person.
The explicit calibrate command still clears Hall and measures a full revolution
from a repeatable edge, even when starting on the magnet.
Homing/calibration is bounded by 180 seconds and 64,000 pulses per phase;
stop cancels it. Calibration measures commanded pulses, so motor stalls or
multiple Hall activations per revolution invalidate the result.

## Map all eight physical positions

Positions stay **1-8 at 45-degree spacing**, including the missing sensor.
Choose position 1 and the numbering direction so that home is halfway between
your physical positions **5 and 6**. Do not renumber the seven working sensors.

1. Send sensors to check enabled sensors. Hardware IDs are 0-7 and are distinct
   from physical positions.
2. Send map. Clear the area within 35 cm of every enabled sensor.
3. At Ready for physical position 1, hold your hand 5-20 cm from that sensor
   for 0.4 seconds. After Captured, remove your hand and wait for Ready.
   A valid range beyond 35 cm clears after 0.4 seconds; fresh weak-return
   readings clear after 1 second when the background is out of range.
4. Continue around physical positions 1 through 8. At the obstructed/disabled
   position, send skip instead of covering a sensor. For example, if physical
   position 6 is missing, capture 1-5, send skip at 6, then capture 7-8.
5. With one disabled sensor, setup needs seven captures plus one skip. It saves
   automatically when all eight physical positions have been assigned.
6. Send order to check the positions, then home to enable automatic behavior.

skip reserves the current physical position for a disabled hardware ID.
It refuses to skip more positions than there are disabled sensors. With multiple
disabled sensors it assigns their IDs in ascending order; if you enable them
later, remap their physical locations.

No automatic movement occurs during setup. Cancel, stop, loss of fresh sensor
data, or 60 seconds without progress aborts mapping and retains the previous map.
If setup is blocked, it reports obstructing hardware IDs/distances or the last
captured sensor status every two seconds, even with DEBUG disabled. Another
enabled sensor seeing an object within 35 cm still blocks readiness. Hardware
errors, stale data and invalid statuses other than SignalFail do not count as
hand removal. For a persistent range error, briefly hold a hand 40-60 cm away.
Only valid near readings can capture a new position.

## Automatic behavior

- Home corresponds to position 5.5, halfway between physical positions 5 and 6.
- Once homed and mapped, a valid smoothed distance under 1,000 mm is considered
  occupied. The same nearest position must remain selected for 350 ms.
- The mirror moves to the direction 180 degrees opposite that physical position,
  taking the shorter rotation. Example: person at position 1 -> mirror at 5;
  person at 5 -> mirror at 1.
- When several sensors detect people/objects, the nearest distance wins. These
  distance sensors do not distinguish people from walls or furniture.
- When no person is detected, it holds its last position. Coils stay enabled
  after homing and automatic moves to retain the reference.
- Sensor reads pause during motor motion to avoid delaying software STEP pulses.
  The next target is evaluated after arrival; this is not continuous tracking.
- stop/cancel, failed motion, and the motor test invalidate homing. Send home
  again to enable automatic movement. Completing mapping also requires home.
- No automatic movement occurs without a saved map. Redo old seven-position
  maps using the eight-position procedure above.

## Configuration

Edit include/Config.h, then rebuild/upload:

| Setting | Purpose |
| --- | --- |
| disabledSensorMask | Hardware IDs to disable; currently ID 5 (XSHUT 10) |
| homeOnStartup | Automatically home after initialization |
| mirrorStepsPerRevolution | STEP pulses per mirror revolution; calibrate to measure |
| motorPositionDirection | +1 follows physical numbering; -1 reverses it |
| mirrorMoveSpeed | Automatic/test maximum speed in pulses/second |
| mirrorAcceleration | Automatic/test acceleration in pulses/second squared |
| homingSpeed | Constant homing/calibration speed in pulses/second |

Use disabledSensorMask = 0 to enable all sensors, (1u << 5) to disable ID 5,
or (1u << 5) | (1u << 6) to disable IDs 5 and 6. Disabled sensors are held in
reset and ignored for detection and mapping clearance. At least one must be enabled.

Detection thresholds are lowerTreshold in src/Sensors.cpp, indexed by physical
position. Set DEBUG to true or false near the top of include/Config.h, then rebuild and
upload. This flag controls automatic movement messages; they
are printed only when a new movement starts. Each includes an uptime timecode
(HH:MM:SS.mmm, since reset), the detected physical zone, and its smoothed distance
in millimetres. Setup and diagnostic output remains
visible independently of DEBUG.

## Commands

| Command | Result |
| --- | --- |
| map | Stop automatic behavior and map eight physical positions |
| skip | During mapping, reserve the current position for a disabled sensor |
| order | Print the map, including disabled positions |
| sensors | Report sensor health/ranges and the Hall input |
| home | Find the Hall reference; enable automatic behavior if a map exists |
| calibrate | Home, then measure pulses in one full mirror revolution |
| motor | Send 400 pulses out and back; requires home afterwards |
| stop / cancel | Cancel movement/setup and disable automatic behavior |
| help | Show available commands |

After stopping a motor command, allow sensor samples to resume briefly before map.
To run a different command during movement, send stop first.

## Wiring and troubleshooting

| Signal | ItsyBitsy pin |
| --- | --- |
| STEP | MISO |
| DIR | MOSI |
| ENABLE | 5, active HIGH as configured for this installation |
| Hall | SCK, active LOW with internal pull-up |
| Sensor XSHUT, IDs 0-7 | A0, A1, A2, A3, 9, 10, 11, 12 |
| Sensor I2C addresses | 0x2A-0x31 |

The ENABLE polarity remains HIGH as set in the working project.
If there is no movement, check driver power, common ground, ENABLE polarity,
STEP/DIR wiring, coil pairs and current setting. Commanded step counts do not
prove actual rotation. A failed sensor initialization requires a reset after
fixing wiring.

## Verification

The firmware builds for itsybitsy_m4. test/motion_math.cpp contains compile-time
checks for wraparound, the shortest rotation, the 5.5 home offset, opposite
targets and reversed direction. Run them with an available C++ compiler:

```text
arm-none-eabi-g++ -std=c++11 -Iinclude -fsyntax-only test/motion_math.cpp
```

Hardware checks still needed: calibrate the revolution count, verify direction,
map the disabled physical position, check opposite targets at positions 1 and 5,
check stop remains stopped, and check a missing Hall signal causes a timeout.

References: [Pololu range validity and nonblocking reads](https://github.com/pololu/vl53l1x-arduino),
[FlashStorage behavior](https://github.com/cmaglie/FlashStorage).

