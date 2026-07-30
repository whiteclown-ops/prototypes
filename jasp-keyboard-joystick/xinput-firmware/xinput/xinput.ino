#include <XInput.h>

#include <EEPROM.h>

#include "joystick_core.h"

// Imperative shell: the only place hardware is touched. Pure logic lives in
// joystick_core.h / joystick_core.cpp.

const boolean InvertLeftXAxis = false;
const boolean InvertLeftYAxis = true;

const int Pin_LeftJoyX = A1;
const int Pin_LeftJoyY = A0;

const int Pin_Calibrate = 15;      // press to toggle calibration (INPUT_PULLUP, LOW = pressed)
const int Pin_CalibrationLed = 2;  // external LED, active-HIGH (anode to pin, cathode to GND via resistor)
const int CalibrationEepromAddress = 0;

const unsigned long TickIntervalMicros = 1000;  // 1000 Hz target tick rate

struct ButtonConfig {
  int pin;
  uint8_t xinputButtonId;  // value from XInput.h
};

const ButtonConfig ButtonConfigs[] = {
  { 14, BUTTON_A },
};

static_assert(sizeof(ButtonConfigs) / sizeof(ButtonConfigs[0]) == NumGamepadButtons,
              "ButtonConfigs size must match NumGamepadButtons in joystick_core.h");

void setup() {
  for (int buttonIndex = 0; buttonIndex < NumGamepadButtons; buttonIndex++) {
    pinMode(ButtonConfigs[buttonIndex].pin, INPUT_PULLUP);
  }
  pinMode(Pin_Calibrate, INPUT_PULLUP);
  pinMode(Pin_CalibrationLed, OUTPUT);
  digitalWrite(Pin_CalibrationLed, LOW);  // active-HIGH; LOW = off

  XInput.setJoystickRange(-JoystickOutputMax, JoystickOutputMax);  // signed range; center = 0
  XInput.setAutoSend(false);                                       // wait for all controls before sending
  XInput.begin();
}

// Confirms a power-on EEPROM reset. Runs once at boot, so the delays are harmless.
static void blinkResetFeedback() {
  for (int blink = 0; blink < 3; blink++) {
    digitalWrite(Pin_CalibrationLed, HIGH);  // active-HIGH; HIGH = on
    delay(120);
    digitalWrite(Pin_CalibrationLed, LOW);  // off
    delay(120);
  }
}

// Signed-cast subtraction handles micros() wrap (~71 min).
static boolean isTimeForNextTick(unsigned long& nextTickMicros) {
  if ((long)(micros() - nextTickMicros) < 0) {
    return false;
  }
  nextTickMicros += TickIntervalMicros;
  if ((long)(micros() - nextTickMicros) > 0) {
    nextTickMicros = micros() + TickIntervalMicros;  // overran a slot; resync, don't burst
  }
  return true;
}

static Inputs readInputs() {
  Inputs inputs;
  inputs.nowMillis = millis();
  inputs.joystickX = analogRead(Pin_LeftJoyX);
  inputs.joystickY = analogRead(Pin_LeftJoyY);
  for (int buttonIndex = 0; buttonIndex < NumGamepadButtons; buttonIndex++) {
    inputs.gamepadButtonRaw[buttonIndex] = digitalRead(ButtonConfigs[buttonIndex].pin);
  }
  inputs.calibrationButtonRaw = digitalRead(Pin_Calibrate);
  return inputs;
}

static void applyOutputs(const Outputs& outputs, const State& state) {
  for (int buttonIndex = 0; buttonIndex < NumGamepadButtons; buttonIndex++) {
    XInput.setButton(ButtonConfigs[buttonIndex].xinputButtonId, outputs.gamepadButtons[buttonIndex]);
  }
  XInput.setJoystickX(JOY_LEFT, outputs.joystickX, InvertLeftXAxis);
  XInput.setJoystickY(JOY_LEFT, outputs.joystickY, InvertLeftYAxis);
  digitalWrite(Pin_CalibrationLed, outputs.calibrationLed ? HIGH : LOW);  // active-HIGH

  if (outputs.saveCalibration) {
    EEPROM.put(CalibrationEepromAddress, state.calibration);
  }
  XInput.send();
}

void loop() {
  static boolean initialized = false;
  static State state;
  static unsigned long nextTickMicros = 0;

  if (!initialized) {
    boolean calibrationHeldAtBoot = (digitalRead(Pin_Calibrate) == LOW);  // held at power-on = reset
    Calibration storedCalibration;
    EEPROM.get(CalibrationEepromAddress, storedCalibration);
    Calibration initialCalibration = resolveInitialCalibration(storedCalibration, calibrationHeldAtBoot);
    if (calibrationHeldAtBoot) {
      EEPROM.put(CalibrationEepromAddress, initialCalibration);  // persist reset to defaults
      blinkResetFeedback();
    }
    state = makeInitialState(initialCalibration, calibrationHeldAtBoot);
    nextTickMicros = micros();
    initialized = true;
  }

  if (!isTimeForNextTick(nextTickMicros)) {
    return;
  }

  Inputs inputs = readInputs();
  TickResult result = computeTick(state, inputs);
  state = result.state;
  applyOutputs(result.outputs, state);
}
