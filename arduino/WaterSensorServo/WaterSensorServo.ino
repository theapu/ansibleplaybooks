/*
 * WaterSensorServo - drive a servo from an HW-038 water level sensor.
 *
 * The HW-038 is an analog sensor: a comb of interleaved traces whose
 * resistance drops as more of the comb is submerged. Reading its S pin on an
 * analog input gives a value that tracks water depth (or, used as a rain
 * sensor, how wet the board is).
 *
 * Wiring (Arduino Uno / Nano)
 *   HW-038  S  -> A0            (analog in, NOT a digital pin)
 *   HW-038  +  -> D7            (see SENSOR_POWER_PIN note below)
 *   HW-038  -  -> GND
 *   Servo signal (orange/yellow) -> D9
 *   Servo V+     (red)           -> Arduino 5V (small servos only, see below)
 *   Servo GND    (brown/black)   -> GND
 *
 * Powering the sensor from a digital pin instead of 5V lets the sketch keep it
 * off between samples. Constant DC across the traces electroplates them away
 * within days of continuous immersion; duty-cycling it makes the board last.
 * Wire + straight to 5V and set SENSOR_POWER_PIN to -1 if you prefer.
 *
 * A micro servo such as an SG90 driving a light load runs fine off the Arduino
 * 5V pin, provided the board is fed from a decent USB supply rather than a
 * laptop port. Fit a 470-1000uF electrolytic across the servo's V+ and GND,
 * close to the servo, with a 100nF ceramic in parallel: it supplies the startup
 * inrush locally instead of sagging the 5V rail. That rail is also the ADC
 * reference, so a sagging rail shifts the very readings that position the
 * servo. Anything larger (MG996R and friends, ~2.5A stalled) needs its own 5V
 * supply with the ground tied to the Arduino's.
 *
 * Two behaviours, chosen with MODE below:
 *   MODE_PROPORTIONAL - servo angle follows the water level continuously,
 *                       e.g. a gauge needle.
 *   MODE_THRESHOLD    - servo snaps between two positions when the level
 *                       crosses a threshold, e.g. a valve or a lid.
 */

#include <Servo.h>

#define MODE_PROPORTIONAL 0
#define MODE_THRESHOLD    1
#define MODE MODE_PROPORTIONAL

const uint8_t SENSOR_PIN       = A0;
const uint8_t SERVO_PIN        = 9;
const int8_t  SENSOR_POWER_PIN = 7;   // -1 if the sensor is wired to 5V

// Calibration. Run the sketch once with the serial monitor open at 9600 baud:
// note the reading with the sensor dry, and the reading at the water level you
// want to count as "full", then put those numbers here.
const int DRY_VALUE = 0;
const int WET_VALUE = 600;

// Servo travel. Swap the two to reverse the direction of rotation.
const int ANGLE_AT_DRY = 0;
const int ANGLE_AT_WET = 180;

// Proportional mode only: ignore movements smaller than this, so sensor noise
// does not leave the servo buzzing between neighbouring degrees.
const int ANGLE_DEADBAND = 2;

// Threshold mode only: the level that trips the servo, and the dead band that
// keeps it from chattering when the reading sits right on the line.
const int TRIP_VALUE  = 300;
const int HYSTERESIS  = 40;

const unsigned long SAMPLE_INTERVAL_MS = 500;
const unsigned long SENSOR_SETTLE_MS   = 10;   // let the sensor stabilise after power-up
const uint8_t       SAMPLES_PER_READ   = 8;    // averaged, to damp noise

Servo servo;
int currentAngle = -1;    // -1 = not yet positioned
bool tripped = false;     // threshold mode state

// Power the sensor only for the moment it takes to read it, then average a
// handful of samples to smooth out mains hum and splashing.
int readLevel() {
  if (SENSOR_POWER_PIN >= 0) {
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    delay(SENSOR_SETTLE_MS);
  }

  long total = 0;
  for (uint8_t i = 0; i < SAMPLES_PER_READ; i++) {
    total += analogRead(SENSOR_PIN);
    delay(2);
  }

  if (SENSOR_POWER_PIN >= 0) {
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }

  return (int)(total / SAMPLES_PER_READ);
}

// Only writes when the target actually changed, so the servo is not
// continuously commanded to a position it already holds.
void moveTo(int angle) {
  angle = constrain(angle, 0, 180);
  if (angle == currentAngle) {
    return;
  }
  servo.write(angle);
  currentAngle = angle;
}

void setup() {
  Serial.begin(9600);

  if (SENSOR_POWER_PIN >= 0) {
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }

  servo.attach(SERVO_PIN);
  moveTo(ANGLE_AT_DRY);
  delay(500);   // give the servo time to reach its starting position

  Serial.println(F("HW-038 water sensor -> servo"));
  Serial.print(F("Calibration: dry="));
  Serial.print(DRY_VALUE);
  Serial.print(F(" wet="));
  Serial.println(WET_VALUE);
}

void loop() {
  int level = readLevel();

#if MODE == MODE_PROPORTIONAL
  // Clamp before mapping so readings outside the calibrated range do not push
  // the servo past its end stops.
  int clamped = constrain(level, DRY_VALUE, WET_VALUE);
  int angle = map(clamped, DRY_VALUE, WET_VALUE, ANGLE_AT_DRY, ANGLE_AT_WET);
  if (currentAngle < 0 || abs(angle - currentAngle) >= ANGLE_DEADBAND) {
    moveTo(angle);
  }
#else
  // Trip above TRIP_VALUE + HYSTERESIS, release below TRIP_VALUE - HYSTERESIS.
  // Between the two the state is left alone.
  if (!tripped && level > TRIP_VALUE + HYSTERESIS) {
    tripped = true;
    moveTo(ANGLE_AT_WET);
  } else if (tripped && level < TRIP_VALUE - HYSTERESIS) {
    tripped = false;
    moveTo(ANGLE_AT_DRY);
  }
#endif

  Serial.print(F("level="));
  Serial.print(level);
  Serial.print(F(" angle="));
  Serial.println(currentAngle);

  delay(SAMPLE_INTERVAL_MS);
}
