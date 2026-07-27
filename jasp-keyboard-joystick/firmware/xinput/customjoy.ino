#include <XInput.h>

const boolean InvertLeftXAxis   = false;
const boolean InvertLeftYAxis   = false;

const int ADC_Max = 1023;  // 10 bit

const int Pin_LeftJoyX  = A1;
const int Pin_LeftJoyY  = A0;

const unsigned long DebounceDelay = 25;  // ms

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

void updateButton(DebouncedButton &button) {
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

  XInput.setJoystickRange(0, ADC_Max);  // Set joystick range to the ADC
  XInput.setAutoSend(false);  // Wait for all controls before sending

  XInput.begin();
}

void loop() {
  updateButtons();

  int leftJoyX = analogRead(Pin_LeftJoyX);
  int leftJoyY = analogRead(Pin_LeftJoyY);

  XInput.setJoystickX(JOY_LEFT, leftJoyX, InvertLeftXAxis);
  XInput.setJoystickY(JOY_LEFT, leftJoyY, InvertLeftYAxis);

  XInput.send(); // Send control data
}
