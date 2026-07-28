#include "core.h"

// ---------------------------------------------------------------------------
// Pure core bodies. Each is a pure function of its arguments: no hardware,
// no millis(), no side effects.
// ---------------------------------------------------------------------------

// Advance a debouncer by one tick. `now` supplied by caller (no millis()).
Debouncer debounceStep(Debouncer d, boolean raw, unsigned long now) {
  if (raw != d.previousRaw) {
    d.previousChangeTime = now;
    d.previousRaw = raw;
  }
  if ((now - d.previousChangeTime) > DebounceDelay) {
    d.stable = raw;
  }
  return d;
}

// Map a raw ADC reading to a signed, normalized [-1, 1] value about a center.
// Two-segment so an off-center rest reads exactly 0 and each side scales
// independently. Span guards avoid divide-by-zero.
float normAxis(int raw, int minVal, int center, int maxVal) {
  if (raw >= center) {
    int span = maxVal - center;
    return span > 0 ? constrain((float)(raw - center) / span, 0.0f, 1.0f) : 0.0f;
  } else {
    int span = center - minVal;
    return span > 0 ? constrain((float)(raw - center) / span, -1.0f, 0.0f) : 0.0f;
  }
}

// Radial deadzone: a circle about center. Output ramps from 0 at the deadzone
// edge (no jump) and keeps direction.
Vec2 applyDeadzone(float nx, float ny) {
  float mag = sqrt(nx * nx + ny * ny);
  if (mag <= DeadzoneDecimalFraction) {
    return { 0.0f, 0.0f };
  }
  float outMag = (mag - DeadzoneDecimalFraction) / (1.0f - DeadzoneDecimalFraction);
  if (outMag > 1.0f) outMag = 1.0f;
  float k = outMag / mag; // mag > 0 here; divide by true magnitude preserves direction
  return { nx * k, ny * k };
}

Calibration defaultCalibration() {
  Calibration c;
  c.magic = CalMagic;
  c.x = { 0, JoyCenter, ADC_Max };
  c.y = { 0, JoyCenter, ADC_Max };
  return c;
}

// Accept stored cal only if the magic matches; otherwise safe defaults so an
// uncalibrated board still works.
Calibration validateCal(Calibration c) {
  return c.magic == CalMagic ? c : defaultCalibration();
}

// Widen an axis to include a new reading.
AxisCal expandAxis(AxisCal a, int raw) {
  if (raw < a.minVal) a.minVal = raw;
  if (raw > a.maxVal) a.maxVal = raw;
  return a;
}

State makeState(Calibration cal) {
  State s;
  s.cal = cal;
  s.calibrating = false;
  s.awaitingCenter = false;
  for (int i = 0; i < NumGamepadButtons; i++) {
    s.buttonDeb[i] = { false, false, 0 };
  }
  s.calButtonDeb = { false, false, 0 };
  return s;
}

// The whole tick as one pure transition.
StepResult step(State s, Inputs in) {
  Outputs out;
  out.saveCal = false;
  out.captureCenter = false;

  // Consume an averaged center the shell captured after last tick's request.
  if (s.awaitingCenter) {
    s.cal.x = { in.capturedCenterX, in.capturedCenterX, in.capturedCenterX };
    s.cal.y = { in.capturedCenterY, in.capturedCenterY, in.capturedCenterY };
    s.awaitingCenter = false;
  }

  // Gamepad buttons ("!" because LOW = pressed).
  for (int i = 0; i < NumGamepadButtons; i++) {
    s.buttonDeb[i] = debounceStep(s.buttonDeb[i], !in.buttonRaw[i], in.now);
    out.buttons[i] = s.buttonDeb[i].stable;
  }

  // Calibration button, debounced edge.
  s.calButtonDeb = debounceStep(s.calButtonDeb, !in.calButtonRaw, in.now);
  boolean pressed = s.calButtonDeb.stable;
  if (pressed != s.calibrating) {
    if (pressed) {
      out.captureCenter = true; // shell averages; result arrives next tick
      s.awaitingCenter = true;
    } else {
      s.cal.magic = CalMagic;
      out.saveCal = true; // write only on release; EEPROM has limited cycles
    }
    s.calibrating = pressed;
  }

  // While calibrating, track extremes as the stick sweeps.
  if (s.calibrating) {
    s.cal.x = expandAxis(s.cal.x, in.joyX);
    s.cal.y = expandAxis(s.cal.y, in.joyY);
  }

  // Joystick output. Hold center while calibrating so sweeping doesn't spray input.
  if (s.calibrating) {
    out.joyX = 0;
    out.joyY = 0;
  } else {
    float nx = normAxis(in.joyX, s.cal.x.minVal, s.cal.x.center, s.cal.x.maxVal);
    float ny = normAxis(in.joyY, s.cal.y.minVal, s.cal.y.center, s.cal.y.maxVal);
    Vec2 v = applyDeadzone(nx, ny);
    out.joyX = (int)(v.x * XInput_Max);
    out.joyY = (int)(v.y * XInput_Max);
  }

  out.rxLed = s.calibrating;

  StepResult r = { s, out };
  return r;
}
