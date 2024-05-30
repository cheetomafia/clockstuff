#include <Encoder.h>
#include <DualMC33926MotorShield.h>
#include <DMXSerial.h>
#include <DMXSerial_avr.h>
#include <DMXSerial_megaavr.h>

// HOUR IS MOTOR 1
// MINUTE IS MOTOR 2


// Addresses and pins
const int DMX_ADDR = 500;
const int M1_ENC_A = 2;
const int M1_ENC_B = 5;
const int M2_ENC_A = 3;
const int M2_ENC_B = 11;

// Motor control
DualMC33926MotorShield md;
Encoder m1Enc(M1_ENC_A, M1_ENC_B);
Encoder m2Enc(M2_ENC_A, M2_ENC_B);
const long COUNTS_PER_REV = 48 * 75 - 8;
const long MINUTE_COUNTS_PER_MIDNIGHT = 12 * COUNTS_PER_REV;
const int RESET_SPEED = 170;
const int MINUTE_CLOCK_SPEED = 170;
const int HOUR_CLOCK_SPEED = -150;
const int ENC_THRESH = 15;
const int HOUR_CHECK_THRESH = 100;
const int HOUR_SAG_ADJ = 165;
const int STUTTER_TIME_MS = 4;

// DMX control
int dmxLast = 0;

// Logic
int currentHour = 0;
int currentTime = 1;
int currentSignal = 0;

void halt(int flashTime) {
  pinMode(LED_BUILTIN, OUTPUT);
  while (1) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(flashTime);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(flashTime);
  }
}

void stopOnFault() {
  if (!md.getFault()) return;
  md.setSpeeds(0, 0);
  halt(1000);
}

void waitForSignal() {
  while (1) {
    stopOnFault();
    int nRead = DMXSerial.read(DMX_ADDR);
    if (nRead == dmxLast) continue;
    delay(10);
    if (DMXSerial.read(DMX_ADDR) != nRead) continue;
    dmxLast = nRead;
    if (dmxLast == 0) currentSignal = 0;
    else currentSignal = dmxLast / 5 + 1; // 1 - 52
    break;
  }
}

void gotoTime(int t) {
  if (t < 1 || t > 48 || currentTime == t) return;

  // Reset encoders into correct range
  m1Enc.write(m1Enc.read() % COUNTS_PER_REV);
  m2Enc.write(m2Enc.read() % MINUTE_COUNTS_PER_MIDNIGHT);
  currentHour %= 12;

  // Calculate goal minute position
  long minuteGoal = (t - 1) * (COUNTS_PER_REV / 4);
  if (minuteGoal == 0 || minuteGoal < m2Enc.read())
    minuteGoal += MINUTE_COUNTS_PER_MIDNIGHT;
  minuteGoal -= ENC_THRESH;
  int hourGoal = 0;
 
  bool minuteMoving = true;
  bool hourMoving = false;
  bool haveSet = true; // true to avoid immediately moving hour hand
  while (minuteMoving || hourMoving) {
    stopOnFault();
    if (DMXSerial.read(DMX_ADDR) == 255) break;

    // Run motors for STUTTER_TIME_MS
    // Stuttering motors might work better??
    if (hourMoving) md.setM1Speed(HOUR_CLOCK_SPEED);
    if (minuteMoving) md.setM2Speed(MINUTE_CLOCK_SPEED);
    delay(STUTTER_TIME_MS);
    md.setSpeeds(0, 0);

    // Stop running motors if they're at their goals
    int hour = m1Enc.read();
    long minute = m2Enc.read();
    minuteMoving = minuteMoving && minute < minuteGoal;
    hourMoving = hourMoving && hour < hourGoal;

    // If the minute hand is up top and we've already started moving the hour hand, we're done
    // Or if the minute hand isn't at the top, we're done
    minute %= COUNTS_PER_REV;
    bool atTop = minute < HOUR_CHECK_THRESH || minute > COUNTS_PER_REV - HOUR_CHECK_THRESH;
    haveSet = haveSet && atTop;
    if (haveSet || !atTop) continue;

    // Run hour hand to the next hour position
    haveSet = true;
    hourMoving = true;
    if (currentHour == 0 && hour > 11 * COUNTS_PER_REV / 12) currentHour = 12;
    currentHour++;
    hourGoal = currentHour * COUNTS_PER_REV / 12 - ENC_THRESH - 5;
    if (currentHour > 0 && currentHour <= 6) hourGoal -= HOUR_SAG_ADJ;
    if (currentHour > 6 && currentHour < 8) hourGoal -= HOUR_SAG_ADJ / 2;
    if (currentHour % 12 == 0) hourGoal -= HOUR_SAG_ADJ / 4;
  }
  currentTime = t;
}

void resetHour() {
  md.setM1Speed(RESET_SPEED);
  while (currentSignal != 0) waitForSignal();
  md.setM1Speed(0);
  m1Enc.write(0);
  currentHour = 0; // DON'T FORGET ABOUT THIS WHEN COPYING
  currentTime = 1;
}

void resetMinute() {
  md.setM2Speed(RESET_SPEED);
  while (currentSignal != 0) waitForSignal();
  md.setM2Speed(0);
  m2Enc.write(0);
}


void setup() {
  DMXSerial.init(DMXReceiver, 6);
  md.init();
  m1Enc.write(0);
  m2Enc.write(0);
}

void loop() {
  waitForSignal();
  switch (currentSignal) {
    case 52:
      break;
    case 51:
      resetHour();
      break;
    case 50:
      resetMinute();
      break;
    default:
      gotoTime(currentSignal);
      break;
  }
}
