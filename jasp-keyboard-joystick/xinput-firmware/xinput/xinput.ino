#include <XInput.h>

#include <EEPROM.h>

#include "core.h"

// ---------------------------------------------------------------------------
// Imperative shell: setup() + loop(). The only place hardware is touched.
// Pure logic lives in core.h / core.cpp.
// ---------------------------------------------------------------------------

const boolean InvertLeftXAxis = false;
const boolean InvertLeftYAxis = false;

const int Pin_LeftJoyX = A1;
const int Pin_LeftJoyY = A0;

const int Pin_Calibrate = 15; // hold to calibrate (INPUT_PULLUP, LOW = pressed)
const int CalEEPROMAddr = 0;  // EEPROM base address for stored calibration
const int CalCenterSamples = 16; // samples averaged for center capture

const unsigned long TickIntervalMicros = 1000; // 1000 Hz target tick rate

// Button layout. xinputButton values come from XInput.h.
struct ButtonConfig {
  int pin;
  uint8_t xinputButton;
};

const ButtonConfig ButtonConfigs[] = {
  { 14, BUTTON_A },
};

static_assert(sizeof(ButtonConfigs) / sizeof(ButtonConfigs[0]) == NumGamepadButtons,
              "ButtonConfigs size must match NumGamepadButtons in core.h");

void setup() {
  for (int i = 0; i < NumGamepadButtons; i++) {
    pinMode(ButtonConfigs[i].pin, INPUT_PULLUP);
  }
  pinMode(Pin_Calibrate, INPUT_PULLUP);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  digitalWrite(LED_BUILTIN_RX, HIGH); // RX LED active-LOW; HIGH = off

  XInput.setJoystickRange(-XInput_Max, XInput_Max); // Signed range; center = 0
  XInput.setAutoSend(false); // Wait for all controls before sending
  XInput.begin();
}

void loop() {
  static boolean started = false;
  static State s;
  static Inputs in = { 0 }; // static so capturedCenter carries to next tick
  static unsigned long nextTickMicros = 0;

  if (!started) {
    Calibration raw;
    EEPROM.get(CalEEPROMAddr, raw);
    s = makeState(validateCal(raw));
    nextTickMicros = micros();
    started = true;
  }

  // Pace to TickIntervalMicros. Signed-cast subtraction handles micros() wrap
  // (~71 min). Return early when it's not yet time — loop() just spins again.
  if ((long)(micros() - nextTickMicros) < 0) {
    return;
  }
  nextTickMicros += TickIntervalMicros;
  // If a tick overran (e.g. the calibration center burst), we're already past
  // the next slot — resync instead of firing a rapid catch-up burst.
  if ((long)(micros() - nextTickMicros) > 0) {
    nextTickMicros = micros() + TickIntervalMicros;
  }

  // --- Gather inputs ---
  in.now = millis();
  in.joyX = analogRead(Pin_LeftJoyX);
  in.joyY = analogRead(Pin_LeftJoyY);
  for (int i = 0; i < NumGamepadButtons; i++) {
    in.buttonRaw[i] = digitalRead(ButtonConfigs[i].pin);
  }
  in.calButtonRaw = digitalRead(Pin_Calibrate);

  // --- Pure step ---
  StepResult r = step(s, in);
  s = r.state;

  // --- Apply outputs ---
  for (int i = 0; i < NumGamepadButtons; i++) {
    XInput.setButton(ButtonConfigs[i].xinputButton, r.out.buttons[i]);
  }
  XInput.setJoystickX(JOY_LEFT, r.out.joyX, InvertLeftXAxis);
  XInput.setJoystickY(JOY_LEFT, r.out.joyY, InvertLeftYAxis);
  digitalWrite(LED_BUILTIN_RX, r.out.rxLed ? LOW : HIGH); // active-LOW

  if (r.out.saveCal) {
    EEPROM.put(CalEEPROMAddr, s.cal);
  }
  if (r.out.captureCenter) {
    long sx = 0, sy = 0;
    for (int i = 0; i < CalCenterSamples; i++) {
      sx += analogRead(Pin_LeftJoyX);
      sy += analogRead(Pin_LeftJoyY);
    }
    in.capturedCenterX = sx / CalCenterSamples;
    in.capturedCenterY = sy / CalCenterSamples;
  }

  XInput.send();
}
