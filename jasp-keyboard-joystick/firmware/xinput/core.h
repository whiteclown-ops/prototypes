#ifndef CORE_H
#define CORE_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pure core: types, constants, and function declarations. No hardware, no
// millis(), no side effects. Definitions live in core.cpp.
// ---------------------------------------------------------------------------

// Constants the core computes with.
const int ADC_Max = 1023; // 10 bit
const unsigned long DebounceDelay = 25; // ms
const float DeadzoneDecimalFraction = 0.05; // 5% of full normalized deflection (radial)
const int JoyCenter = ADC_Max / 2; // default zero, zero center
const int XInput_Max = 32767; // signed joystick output range
const uint16_t CalMagic = 0x4A53; // 'JS' — marks EEPROM as holding valid calibration

// Number of gamepad buttons. Must match ButtonConfigs[] in the sketch; a
// static_assert there enforces it at compile time.
const int NumGamepadButtons = 1;

// Per-axis calibrated endpoints; center may sit off ADC_Max/2, range may be narrow.
struct AxisCal {
  int minVal;
  int center;
  int maxVal;
};

struct Calibration {
  uint16_t magic;
  AxisCal x;
  AxisCal y;
};

// Reusable debounce: holds the last stable state and only updates it once the
// raw input has been steady for DebounceDelay.
struct Debouncer {
  boolean previousRaw;
  boolean stable;
  unsigned long previousChangeTime;
};

struct Vec2 {
  float x;
  float y;
};

// All state, threaded through the pure core.
struct State {
  Calibration cal;
  boolean calibrating;    // true while calibration button held
  boolean awaitingCenter; // set on press edge; consumed once shell supplies an averaged center
  Debouncer buttonDeb[NumGamepadButtons];
  Debouncer calButtonDeb;
};

// Everything the core reads this tick. Gathered by the shell.
struct Inputs {
  unsigned long now;
  int joyX;
  int joyY;
  boolean buttonRaw[NumGamepadButtons]; // raw digitalRead (LOW = pressed)
  boolean calButtonRaw;                 // raw digitalRead (LOW = pressed)
  int capturedCenterX; // averaged center from shell; valid the tick after captureCenter was requested
  int capturedCenterY;
};

// Everything the core wants applied this tick. Applied by the shell.
struct Outputs {
  boolean buttons[NumGamepadButtons];
  int joyX;
  int joyY;
  boolean rxLed;
  boolean saveCal;       // shell persists cal to EEPROM
  boolean captureCenter; // shell averages samples, feeds back next tick
};

struct StepResult {
  State state;
  Outputs out;
};

// --- Pure functions ---
Debouncer debounceStep(Debouncer d, boolean raw, unsigned long now);
float normAxis(int raw, int minVal, int center, int maxVal);
Vec2 applyDeadzone(float nx, float ny);
Calibration defaultCalibration();
Calibration validateCal(Calibration c);
AxisCal expandAxis(AxisCal a, int raw);
State makeState(Calibration cal);
StepResult step(State s, Inputs in);

#endif // CORE_H
