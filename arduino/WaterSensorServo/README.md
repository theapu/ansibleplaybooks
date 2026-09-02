# WaterSensorServo

Drives a hobby servo from an **HW-038** water level sensor (the red comb-shaped
board marked `S + -`).

## About the sensor

The HW-038 is an *analog* sensor. Its parallel traces form a variable resistor:
the more of the comb is submerged, the lower the resistance and the higher the
voltage on `S`. Readings therefore track water depth rather than giving a simple
wet/dry bit, so `S` must go to an analog input (`A0`), not a digital pin.

It works as a rain detector too — droplets bridging the traces raise the reading
— though a flat FC-37 rain board covers more area for that job.

## Wiring

| HW-038 | Arduino |
| ------ | ------- |
| `S`    | `A0`    |
| `+`    | `D7`    |
| `-`    | `GND`   |

| Servo | Wire colour | Connection |
| ----- | ----------- | ---------- |
| signal | orange / yellow / white (edge) | `D9` |
| V+     | red (always the middle wire)   | `5V` (small servos only — see below) |
| GND    | brown / black / grey (edge)    | `GND` |

Red is always the middle wire on a 3-wire servo lead; the two outer wires are
signal and ground, ground being the darker of them. Check the connector against
that before plugging it in — swapping red and ground can destroy the servo.

Two things worth doing rather than the obvious shortcut:

- **Power the sensor from a digital pin.** Continuous DC across the traces
  electroplates them away within days of immersion. The sketch powers the sensor
  only for the few milliseconds it takes to read. If you would rather wire `+`
  to `5V`, set `SENSOR_POWER_PIN` to `-1`.
- **Decouple the servo.** An SG90 turning a light load runs fine off the 5V pin,
  as long as the board is fed from a proper USB supply rather than a laptop
  port. Fit a 470–1000µF electrolytic across the servo's V+ and GND, close to
  the servo, with a 100nF ceramic in parallel — it supplies the startup inrush
  locally instead of sagging the 5V rail. That rail is the ADC reference too, so
  a sagging rail shifts the very readings that position the servo, and the noise
  looks like a flaky sensor.

## Servo current

| Servo         | Idle  | Moving, no load | Stall / inrush     |
| ------------- | ----- | --------------- | ------------------ |
| SG90 (micro)  | ~10mA | 100–250mA       | ~700mA, >1A spikes |
| MG996R        | ~10mA | 500mA+          | ~2.5A              |

The 5V pin can supply roughly 500mA on USB (shared with the board itself), less
if you power the Arduino through the barrel jack, since that runs through a
linear regulator that will thermal-cycle under load. An SG90 fits inside that
budget; anything geared or metal-geared does not and needs its own supply.

Never power a servo from a digital pin — those source about 20mA.

## Calibrating

Upload the sketch and open the serial monitor at 9600 baud. Note the reading
with the sensor dry, and again at the level you want to treat as "full", then
set `DRY_VALUE` and `WET_VALUE` to those numbers. The readings depend on water
conductivity, so recalibrate if you switch from tap water to anything else.

## Modes

Set `MODE` at the top of the sketch:

- `MODE_THRESHOLD` (default) — the servo sweeps its full `ANGLE_AT_DRY` →
  `ANGLE_AT_WET` travel (0° → 180° as shipped) as soon as water is detected, and
  returns once the sensor dries. For a valve, a lid, or anything else that is
  simply open or shut. `TRIP_VALUE` is the level that fires it; `HYSTERESIS` is
  the dead band that stops it chattering when the reading sits on the line, so
  it trips above `TRIP_VALUE + HYSTERESIS` and releases below
  `TRIP_VALUE - HYSTERESIS`.
- `MODE_PROPORTIONAL` — the servo angle follows the water level continuously,
  e.g. a gauge needle. `DRY_VALUE`/`WET_VALUE` set the range that maps onto the
  servo's travel.

In threshold mode, set `TRIP_VALUE` just above the dry reading from the serial
monitor — a dry board is rarely a clean zero, and a trip point too close to it
will fire on humidity alone.

The movement is symmetric: the servo returns to `ANGLE_AT_DRY` by itself once
the sensor dries out. For a door, `ANGLE_AT_DRY` is the open position and
`ANGLE_AT_WET` the closed one; swap the two values if your linkage runs the
other way.

`CONFIRM_SAMPLES` requires that many consecutive readings agree before the servo
moves — three at the default sample interval means about 1.5 seconds of settled
readings, so a stray splash cannot slam the door shut and straight back open.
Raise it if the sensor sits somewhere exposed to spray.

## Servo speed

`servo.write()` sends the servo to a position as fast as it can travel; a servo
has no speed input of its own. To slow it, the sketch commands a series of
nearby angles instead of one distant one, stepping `SWEEP_STEP_DEG` degrees and
pausing `SWEEP_STEP_DELAY_MS` between steps.

| `SWEEP_STEP_DELAY_MS` | Time for a full 180° sweep |
| --------------------- | -------------------------- |
| 5   | ~0.9s |
| 15 (default) | ~2.7s |
| 40  | ~7s   |
| 80  | ~14s  |

Leave `SWEEP_STEP_DEG` at 1 — raising it makes the motion coarser and jerkier
without making it faster. Note that a move blocks until it finishes, so the
sensor is not sampled while the servo is travelling.

Below roughly 5ms per step you are asking for movement finer than the servo can
resolve, and it will simply run at full speed.

Swap `ANGLE_AT_DRY` and `ANGLE_AT_WET` to reverse the direction of rotation.
