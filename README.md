# Shy mirror firmware

Adafruit ItsyBitsy M4 Express (SAMD51G19A), built with PlatformIO.
Open Firmware/shy-mirror-fw in VS Code, or run these commands from that folder:

```powershell
pio run
pio run -t upload
pio device monitor
```

The serial monitor uses 115200 baud, local echo and send-on-enter.
Close it before uploading.

## Fixed sensor positions

The order is hardcoded in Firmware/shy-mirror-fw/include/Config.h.
No hand-mapping procedure or saved flash map is required after upload.

| Physical sensor / position | XSHUT pin | Internal hardware ID |
| --- | --- | --- |
| 1 | 9 | 4 |
| 2 | A3 | 3 |
| 3 | A2 | 2 |
| 4 | A1 | 1 |
| 5 | A0 | 0 |
| 6 | 12 | 7 |
| 7 | 11 | 6 |
| 8 | 10 | 5 |

Pin 10 (physical sensor 8, hardware ID 5) remains disabled via disabledSensorMask.
To enable it, set that mask to 0 and upload. All eight physical positions remain
45 degrees apart; disabling a sensor does not renumber the others.
The sensors command reports readings in the physical order above.

## Automatic interactive mode

On startup the firmware initializes sensors and homes the mirror. If the Hall
sensor is already active LOW for 15 ms, homing movement is skipped. Otherwise it
seeks the Hall reference in the positive motor direction.

After successful homing, it prints:
Interactive mode active: mirror moves opposite the detected person.

Home corresponds to physical position 5.5, halfway between positions 5 and 6.
A valid smoothed distance below 1,000 mm detects occupancy. Once the nearest
position remains selected for 100 ms, the mirror moves 180 degrees opposite it,
using the shorter rotation. For example, person at 1 -> mirror at 5, person at
5 -> mirror at 1. The nearest distance wins if several positions are occupied.
Distance sensors can also detect nearby furniture; they do not identify humans.

NO TARGET / weak returns are normal in empty space and never trigger movement.
Stale readings, communication errors and other invalid ranges are ignored for
person detection. When no person is detected, the mirror holds its last position.

Sensors continue updating during automatic movement, and a new confirmed person
position retargets the motor immediately. AccelStepper preserves speed and
decelerates before reversing; the coordinate is normalized only when at rest.
Repeated detections of the same target do not restart the move or its timeout.

TC3 on the SAMD51 services STEP pulses at 40 kHz, independently of foreground
I2C/serial work. TC3 is reserved for the motor; do not reuse it for PWM or another
library. If foreground execution stalls for more than 250 ms, the interrupt
disables the driver and stops pulses; home again after resolving the fault.
Sensor reads remain paused during homing/calibration and the manual motor test.
Coils remain enabled after successful homing and automatic movement.

stop/cancel, failed movement and motor tests disable automatic behavior.
Send home to resume. Homing is bounded by 180 seconds and 64,000 pulses.

## Configuration

Edit Firmware/shy-mirror-fw/include/Config.h, then rebuild/upload.

| Setting | Purpose |
| --- | --- |
| DEBUG | true shows detection distance and uptime when a new movement starts |
| disabledSensorMask | Disabled hardware IDs; currently bit 5 / physical sensor 8 |
| sensorHardwareIdByPosition | Fixed position-to-hardware-ID order |
| homeOnStartup | true starts homing automatically |
| mirrorStepsPerRevolution | Motor STEP pulses per full mirror turn, measured as 15,999 |
| motorPositionDirection | +1 follows increasing positions; -1 reverses them |
| mirrorMoveSpeed | Maximum automatic/test speed, pulses per second |
| mirrorAcceleration | Automatic/test acceleration, pulses per second squared |
| homingSpeed | Constant homing speed, pulses per second |
| sensorTimingBudgetUs | 20,000 us per measurement in Short mode |
| sensorPeriodMs | 30 ms continuous measurement period |
| personConfirmMs | 100 ms stable target confirmation |
| sensorStaleMs | Discard samples older than 100 ms |
| sensorSmoothingDivisor | 2 uses 50% of the new reading (was 10%) |

The configured revolution count is the Hall-measured value of 15,999 pulses. Optional motor-only
calibrate measures one revolution using successive Hall activations. It is
separate from the removed sensor-mapping setup and is never required at startup.
It requires exactly one Hall activation per mirror revolution. Copy its printed
result into mirrorStepsPerRevolution to retain it across resets.
If positive rotation goes opposite to increasing physical positions, use -1 for
motorPositionDirection.

Detection thresholds are lowerTreshold in src/Sensors.cpp, indexed by position.
The faster sensor settings use Short mode (about 1.3 m maximum range) for the
existing 1 m detection threshold. More distant surfaces may report NO TARGET.
Short mode supports a 20 ms budget; use at least 33 ms if changing back to Long
mode. These limits are documented by the
[Pololu library](https://github.com/pololu/vl53l1x-arduino#library-reference).

The configured motor rates are preserved at 3,200 pulses/s and 6,400 pulses/s^2.
Timer-based servicing still needs a bench check for pulse timing, smooth
reversals and missed steps at the mirror's actual mechanical load.

## Serial commands

| Command | Result |
| --- | --- |
| order | Show the fixed sensor positions; map is a read-only alias |
| sensors | Report distances, health, disabled sensors and Hall state |
| home | Home and automatically enter interactive mode |
| stop / cancel | Stop and disable automatic movement until home |
| motor | Test 400 pulses forward/back; home again afterwards |
| calibrate | Optional motor revolution measurement; does not change sensor order |
| help | List commands |

## Wiring

STEP = MISO, DIR = MOSI, ENABLE = pin 5 (active HIGH), Hall = SCK
(active LOW, internal pull-up). I2C addresses remain assigned by internal
hardware ID from 0x2A through 0x31.

If the motor does not move, check driver power, common ground, ENABLE polarity,
STEP/DIR wiring, coil pairs and driver current. Pulses do not prove physical
rotation. Reset after repairing a sensor that failed initialization.

## Validation

Compile-time checks cover the fixed sensor order and opposite-position math:

```text
arm-none-eabi-g++ -std=c++11 -Iinclude -fsyntax-only test/fixed_sensor_order.cpp test/motion_math.cpp
```

Bench-check homing, moving a target between positions while the motor is
turning, smooth reversal, empty-space readings, motor direction and stop. The firmware has been compiled; physical movement requires verification.

