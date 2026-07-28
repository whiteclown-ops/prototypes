#include <XInput.h>

#include <EEPROM.h>

const boolean InvertLeftXAxis = false;
const boolean InvertLeftYAxis = false;

const int ADC_Max = 1023; // 10 bit

const int Pin_LeftJoyX = A1;
const int Pin_LeftJoyY = A0;

const unsigned long DebounceDelay = 25; // ms

const float DeadzoneDecimalFraction = 0.05; // 5% of full normalized deflection (radial)
const int JoyCenter = ADC_Max / 2; // default zero, zero center

const int XInput_Max = 32767; // signed joystick output range

// --- Calibration ---
const int Pin_Calibrate = 15; // hold to calibrate (INPUT_PULLUP, LOW = pressed)
const int CalCenterSamples = 16; // samples averaged for center capture
const int CalEEPROMAddr = 0; // EEPROM base address for stored calibration
const uint16_t CalMagic = 0x4A53; // 'JS' — marks EEPROM as holding valid calibration

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

Calibration cal; // loaded from EEPROM in setup()
boolean calibrating = false; // true while calibration button held

// Reusable debounce: holds the last stable state and only updates it once the
// raw input has been steady for DebounceDelay. Returns the debounced state.
struct Debouncer {
  boolean previousRaw;
  boolean stable;
  unsigned long previousChangeTime;
};

boolean debounce(Debouncer & d, boolean raw) {
  if (raw != d.previousRaw) {
    d.previousChangeTime = millis();
    d.previousRaw = raw;
  }
  if ((millis() - d.previousChangeTime) > DebounceDelay) {
    d.stable = raw;
  }
  return d.stable;
}

struct DebouncedButton {
  const int pin;
  const uint8_t xinputButton;
  Debouncer deb;
};

DebouncedButton gamepadButtons[] = {
  {
    14,
    BUTTON_A,
    {
      false,
      false,
      0
    }
  },
};

const int NumGamepadButtons = sizeof(gamepadButtons) / sizeof(gamepadButtons[0]);

void updateGamepadButton(DebouncedButton & gamepadButton) {
  // Note the "!" to invert the state, because LOW = pressed
  boolean pressed = debounce(gamepadButton.deb, !digitalRead(gamepadButton.pin));
  XInput.setButton(gamepadButton.xinputButton, pressed);
}

// Map a raw ADC reading to a signed, normalized [-1, 1] value about a center.
// Two-segment so an off-center rest reads exactly 0 and each side scales
// independently. Span guards avoid divide-by-zero.
float normAxis(int raw, int minVal, int center, int maxVal) {
  if (raw >= center) {
    int span = maxVal - center;
    return span > 0 ? constrain((float)(raw - center) / span, 0.0 f, 1.0 f) : 0.0 f;
  } else {
    int span = center - minVal;
    return span > 0 ? constrain((float)(raw - center) / span, -1.0 f, 0.0 f) : 0.0 f;
  }
}

// Read both axes, apply a radial deadzone, and send to XInput. The deadzone is
// a circle about center; output ramps from 0 at the deadzone edge (no jump) and
// keeps direction.
void updateLeftJoystick() {
  if (calibrating) {
    // Hold at center so sweeping the stick during calibration doesn't spray input
    XInput.setJoystickX(JOY_LEFT, 0, InvertLeftXAxis);
    XInput.setJoystickY(JOY_LEFT, 0, InvertLeftYAxis);
    return;
  }

  float nx = normAxis(analogRead(Pin_LeftJoyX), cal.x.minVal, cal.x.center, cal.x.maxVal);
  float ny = normAxis(analogRead(Pin_LeftJoyY), cal.y.minVal, cal.y.center, cal.y.maxVal);

  float mag = sqrt(nx * nx + ny * ny);
  if (mag <= DeadzoneDecimalFraction) {
    nx = 0;
    ny = 0;
  } else {
    float outMag = (mag - DeadzoneDecimalFraction) / (1.0 f - DeadzoneDecimalFraction);
    if (outMag > 1.0 f) outMag = 1.0 f;
    float k = outMag / mag; // mag > 0 here; divide by true magnitude preserves direction
    nx *= k;
    ny *= k;
  }

  XInput.setJoystickX(JOY_LEFT, (int)(nx * XInput_Max), InvertLeftXAxis);
  XInput.setJoystickY(JOY_LEFT, (int)(ny * XInput_Max), InvertLeftYAxis);
}

void setupButtonPins() {
  // Set buttons as inputs, using internal pull-up resistors
  for (int i = 0; i < NumGamepadButtons; i++) {
    pinMode(gamepadButtons[i].pin, INPUT_PULLUP);
  }

  pinMode(Pin_Calibrate, INPUT_PULLUP);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  setRxLed(false);
}

void updateGamepadButtons() {
  // Read, debounce, and set each gamepad button
  for (int i = 0; i < NumGamepadButtons; i++) {
    updateGamepadButton(gamepadButtons[i]);
  }
}

// RX LED on the Pro Micro is active-LOW.
void setRxLed(boolean on) {
  digitalWrite(LED_BUILTIN_RX, on ? LOW : HIGH);
}

// Load stored calibration; fall back to safe defaults (full range, mid center)
// if EEPROM has never been written, so an uncalibrated board still works.
void loadCalibration() {
  EEPROM.get(CalEEPROMAddr, cal);
  if (cal.magic != CalMagic) {
    cal.magic = CalMagic;
    cal.x = {
      0,
      JoyCenter,
      ADC_Max
    };
    cal.y = {
      0,
      JoyCenter,
      ADC_Max
    };
  }
}

// Average several samples at rest to set each axis center; seed min/max to it.
void captureCenter() {
  long sx = 0, sy = 0;
  for (int i = 0; i < CalCenterSamples; i++) {
    sx += analogRead(Pin_LeftJoyX);
    sy += analogRead(Pin_LeftJoyY);
  }
  int cx = sx / CalCenterSamples;
  int cy = sy / CalCenterSamples;
  cal.x = {
    cx,
    cx,
    cx
  };
  cal.y = {
    cy,
    cy,
    cy
  };
}

// Debounced calibration button. Press = capture center + start recording
// extremes (RX LED on). While held, track per-axis min/max as the stick sweeps.
// Release = persist to EEPROM (RX LED off).
void updateCalibration() {
  static Debouncer deb = {
    false,
    false,
    0
  };
  static boolean wasPressed = false;

  boolean pressed = debounce(deb, !digitalRead(Pin_Calibrate)); // LOW = pressed

  if (pressed != wasPressed) { // debounced edge
    wasPressed = pressed;
    if (pressed) {
      captureCenter();
      calibrating = true;
      setRxLed(true);
    } else {
      cal.magic = CalMagic;
      EEPROM.put(CalEEPROMAddr, cal); // write only on release; EEPROM has limited cycles
      calibrating = false;
      setRxLed(false);
    }
  }

  if (calibrating) {
    int rx = analogRead(Pin_LeftJoyX);
    int ry = analogRead(Pin_LeftJoyY);
    if (rx < cal.x.minVal) cal.x.minVal = rx;
    if (rx > cal.x.maxVal) cal.x.maxVal = rx;
    if (ry < cal.y.minVal) cal.y.minVal = ry;
    if (ry > cal.y.maxVal) cal.y.maxVal = ry;
  }
}

void setup() {
  setupButtonPins();
  loadCalibration();

  XInput.setJoystickRange(-XInput_Max, XInput_Max); // Signed range; center = 0
  XInput.setAutoSend(false); // Wait for all controls before sending

  XInput.begin();
}

void loop() {
  updateGamepadButtons();
  updateCalibration();
  updateLeftJoystick();

  XInput.send(); // Send control data
}
