# XInput Firmware Installation Guide

## Prerequisites
- Download and install [Arduino IDE](https://www.arduino.cc/en/software/)
- During first launch, allow Arduino IDE to install drivers when prompted

## Install the XInput USB Core for Arduino AVR

1. Go to [github.com/dmadison/ArduinoXInput_AVR](https://github.com/dmadison/ArduinoXInput_AVR)
2. Follow the repo's install instructions to add the board manager URL and install the **XInput AVR Boards** package via **Tools → Board → Boards Manager**
3. Run the included `XInput_Boards_Firmware` uploader sketch (or use the board's Serial/DFU install method per the repo instructions) to flash the XInput USB core onto your Pro Micro's 16u2/32u4 USB controller

## Setup

1. Open [customjoy.ino](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/firmware/xinput/customjoy.ino) in Arduino IDE
2. Plug your Pro Micro into your PC
3. In the Arduino IDE board dropdown, select **Arduino Leonardo (XInput)**

## Compile & Upload

1. Press the **checkmark icon** to compile the firmware
2. Press the **arrow icon** to upload it to your board

### Upload Warning

Due to the nature of how the XInput USB mode works, Arduinos that have XInput sketches on them will not automatically reset when programmed by the IDE! You will need to reset the board by hand every time you upload new code.
See the [dmadison/ArduinoXInput_AVR repo](https://github.com/dmadison/ArduinoXInput_AVR) for more details on this.

## Verify It's Working

1. Open the Windows Start menu and search for **"Set up USB game controllers"**
2. **Controller (XBOX 360 For Windows)** should appear in the list
3. Click **Properties** and confirm the analog joystick is responding

## Game Compatibility

Since the board enumerates as an Xbox 360 controller, most games should recognize it directly with no remapper needed.
