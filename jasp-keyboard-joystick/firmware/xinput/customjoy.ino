#include <XInput.h>

const boolean InvertLeftXAxis = false;
const boolean InvertLeftYAxis = false;

const int ADC_Max = 1023; // 10 bit

const int Pin_LeftJoyX = A1;
const int Pin_LeftJoyY = A0;

const unsigned long DebounceDelay = 25; // ms

const float DeadzoneDecimalFraction = 0.05; // 5% of full normalized deflection (radial)
const int JoyCenter = ADC_Max / 2; // default zero, zero center

const int XInput_Max = 32767; // signed joystick output range

struct DebouncedButton {
  const int pin;
  const uint8_t xinputButton;
  boolean pressed;
  boolean previousPressed;
  unsigned long previousChangeTime;
};

DebouncedButton buttons[] = {
  { 14, BUTTON_A, false, false, 0 },
};

const int NumButtons = sizeof(buttons) / sizeof(buttons[0]);

void updateButton(DebouncedButton & button) {
  // Note the "!" to invert the state, because LOW = pressed
  boolean currentPressed = !digitalRead(button.pin);

  if (currentPressed != button.previousPressed) {
    button.previousChangeTime = millis();
  }

  if ((millis() - button.previousChangeTime) > DebounceDelay) {
    button.pressed = currentPressed;
  }

  button.previousPressed = currentPressed;

  XInput.setButton(button.xinputButton, button.pressed);
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
  float nx = normAxis(analogRead(Pin_LeftJoyX), 0, JoyCenter, ADC_Max);
  float ny = normAxis(analogRead(Pin_LeftJoyY), 0, JoyCenter, ADC_Max);

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
  for (int i = 0; i < NumButtons; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
}

void updateButtons() {
  // Read, debounce, and set each button
  for (int i = 0; i < NumButtons; i++) {
    updateButton(buttons[i]);
  }
}

void setup() {
  setupButtonPins();

  XInput.setJoystickRange(-XInput_Max, XInput_Max); // Signed range; center = 0
  XInput.setAutoSend(false); // Wait for all controls before sending

  XInput.begin();
}

void loop() {
  updateButtons();

  updateLeftJoystick();

  XInput.send(); // Send control data
}
