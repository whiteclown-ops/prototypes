# XInput Firmware Installation Guide

## Prerequisites
- Download and install [Arduino IDE](https://www.arduino.cc/en/software/)
- During first launch, allow Arduino IDE to install drivers when prompted

## Install the XInput USB Core for Arduino AVR

1. Go to [github.com/dmadison/ArduinoXInput_AVR](https://github.com/dmadison/ArduinoXInput_AVR)
2. Follow the repo's install instructions to add the board manager URL and install the **XInput AVR Boards** package via **Tools → Board → Boards Manager**
3. Run the included `XInput_Boards_Firmware` uploader sketch (or use the board's Serial/DFU install method per the repo instructions) to flash the XInput USB core onto your Pro Micro's 16u2/32u4 USB controller

## Wiring

| Signal              | Pin  | Notes                                      |
|---------------------|------|--------------------------------------------|
| Left joystick X     | A1   | analog                                     |
| Left joystick Y     | A0   | analog                                     |
| Gamepad button (A)  | 14   | `INPUT_PULLUP`, wire to GND, LOW = pressed |
| Calibrate button    | 15   | `INPUT_PULLUP`, wire to GND, LOW = pressed |

Both buttons use the internal pull-up, so each just needs a switch to ground — no external resistors.

## Setup

The sketch is a folder, not a single file. It contains three files that must stay together in the same directory:

- `xinput.ino`
- `joystick_core.cpp`
- `joystick_core.h`

1. Open [xinput.ino](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/xinput-firmware/xinput/xinput.ino) in Arduino IDE (opening the `.ino` loads the whole `xinput/` folder)
2. Plug your Pro Micro into your PC
3. In the Arduino IDE board dropdown, select **Arduino Leonardo (XInput)**

## Compile & Upload

1. Press the **checkmark icon** to compile the firmware
2. Press the **arrow icon** to upload it to your board

### Upload Warning

Due to the nature of how the XInput USB mode works, Arduinos that have XInput sketches on them will not automatically reset when programmed by the IDE! You will need to reset the board by hand every time you upload new code.
See the [dmadison/ArduinoXInput_AVR repo](https://github.com/dmadison/ArduinoXInput_AVR) for more details on this.

## Calibration

The firmware learns each joystick's true center and travel range and stores them in EEPROM, so calibration survives power cycles and reflashes of the same board. Run it once after assembly, or any time the stick drifts or feels off-center.

1. **Center the stick** and let it rest.
2. **Press and hold the calibrate button (pin 15).** The center is captured the instant you press, so don't touch the stick until it's held. The **RX LED lights up** to show calibration is active.
3. **Sweep the stick through its full range** while holding — push into all four corners and around the edge so it sees the true min and max on both axes.
4. **Release the calibrate button.** The captured min / center / max are saved to EEPROM.

The RX LED off means calibration is not active. If you upload fresh firmware to a board with no stored calibration, it falls back to a default center and full ADC range until you calibrate.

### Reset to Defaults

To wipe a stored calibration and return to the defaults (center at mid-scale, full ADC range):

1. **Hold the calibrate button (pin 15) while powering on** the board (plug it in, or reset it, with the button already down).
2. The **RX LED blinks three times** to confirm the EEPROM was reset to defaults.
3. **Release the button.**

The boot-time hold is treated only as a reset — it does not start a new calibration. To recalibrate afterward, release the button and then run the normal [Calibration](#calibration) steps.

## Verify It's Working

1. Open the Windows Start menu and search for **"Set up USB game controllers"**
2. **Controller (XBOX 360 For Windows)** should appear in the list
3. Click **Properties** and confirm the analog joystick is responding
4. If the resting position reads off-center or the stick can't reach the edges, run [Calibration](#calibration)

## Game Compatibility

Since the board enumerates as an Xbox 360 controller, most games should recognize it directly with no remapper needed.
