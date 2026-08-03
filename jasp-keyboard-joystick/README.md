## Jasp Keyboard Joystick

The Keyboard Joystick solves the WASD movement problem for PC gamers. You can also map it to mouse movement and effectively use your PC with one hand. It's a relatively simple project that requires minimal soldering. I originally designed this back in 2022 and have since been evolving it into a full keypad. This design is far from perfect, but I would love for you to try it and let me know how it works for you.

![3d-printed](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/img/printed-1.png)

## Build Guide

There is no detailed build guide available at the moment, but you can see the whole build process in [this video](https://www.youtube.com/watch?v=S8SKIpWGIe8&t=1s)

## Wiring Diagram

![Wiring_diagram](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/ProMicro_Wiring_Diagram.png)


## Firmware

There are two firmware options:

### Joystick (basic)
A quick and dirty firmware available [here](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/firmware). It's super basic but it works. Enumerates the board as a generic USB joystick. [Installation guide](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/firmware/Firmware_Installation_Guide.md)

### XInput (Xbox 360 controller)
A newer firmware available [here](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/xinput-firmware) that enumerates the board as an Xbox 360 controller, so most games recognize it directly with no remapper needed. It adds debounced gamepad buttons, a radial deadzone, 1000hz polling, and a stick calibration stored in EEPROM. [Installation guide](https://github.com/multifex/prototypes/blob/main/jasp-keyboard-joystick/xinput-firmware/XInput_Firmware_Installation_Guide.md)
> Note: This firmware does make use of an extra button for calibarion and an extra LED; But, the code can be modified to use the joystick button for calibarion and the native rx or tx LED on the Ardunio Micro for feedback. Although this is not recommended as it is difficult to click the joystick perfectly center, as the start of the calibarion mode expects.

## License

Files in this repository are released under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

You are free to use, modify, and share these files for **personal and educational use** with attribution. **Commercial use is not permitted** under this license.

For commercial licensing inquiries, please reach out.  


## Follow Along

I document the design and build process of all my projects here: [@JaspMakes](https://youtube.com/@jaspmakes)
