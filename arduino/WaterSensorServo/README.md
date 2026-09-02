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

| Servo           | Connection |
| --------------- | ---------- |
| signal (yellow) | `D9`       |
| V+ (red)        | `5V` (small servos only — see below) |
| GND (brown)     | `GND`      |

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

- `MODE_PROPORTIONAL` (default) — the servo angle follows the water level
  continuously, e.g. a gauge needle.
- `MODE_THRESHOLD` — the servo snaps between two positions when the level
  crosses `TRIP_VALUE`, e.g. a valve or a lid. `HYSTERESIS` sets the dead band
  that stops it chattering when the reading sits on the line.

Swap `ANGLE_AT_DRY` and `ANGLE_AT_WET` to reverse the direction of rotation.
