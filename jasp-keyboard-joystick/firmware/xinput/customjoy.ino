#include <XInput.h>

// Setup
const boolean InvertLeftXAxis   = false;
const boolean InvertLeftYAxis   = false;

const int ADC_Max = 1023;  // 10 bit

// Joystick Pins
const int Pin_LeftJoyX  = A1;
const int Pin_LeftJoyY  = A0;

// Button Pins
const int Pin_ButtonA = 14;

void setup() {
  // Set buttons as inputs, using internal pull-up resistors
  pinMode(Pin_ButtonA, INPUT_PULLUP);

  XInput.setJoystickRange(0, ADC_Max);  // Set joystick range to the ADC
  XInput.setAutoSend(false);  // Wait for all controls before sending

  XInput.begin();
}

void loop() {
  // Read pin values and store in variables
  // (Note the "!" to invert the state, because LOW = pressed)
  boolean buttonA = !digitalRead(Pin_ButtonA);

  // Set XInput buttons
  XInput.setButton(BUTTON_A, buttonA);

  // Set left joystick
  int leftJoyX = analogRead(Pin_LeftJoyX);
  int leftJoyY = analogRead(Pin_LeftJoyY);

  XInput.setJoystickX(JOY_LEFT, leftJoyX, InvertLeftXAxis);
  XInput.setJoystickY(JOY_LEFT, leftJoyY, InvertLeftYAxis);

  // Send control data to the computer
  XInput.send();
}
