#ifndef _JOYSTICK_HID_H_
#define _JOYSTICK_HID_H_

#include <Arduino.h>

const uint32_t BUTTON_1         = 0b1;
const uint32_t BUTTON_2         = 0b10;
const uint32_t BUTTON_3         = 0b100;
const uint32_t BUTTON_4         = 0b1000;
const uint32_t BUTTON_L_BUMPER  = 0b10000;
const uint32_t BUTTON_R_BUMPER  = 0b100000;
const uint32_t BUTTON_L_TRIGGER = 0b1000000;
const uint32_t BUTTON_R_TRIGGER = 0b10000000;
const uint32_t BUTTON_MENU_1    = 0b100000000;
const uint32_t BUTTON_MENU_2    = 0b1000000000;
const uint32_t BUTTON_L_THUMB   = 0b10000000000;
const uint32_t BUTTON_R_THUMB   = 0b100000000000;

const int AXIS_LEFT_STICK_X = 0;
const int AXIS_LEFT_STICK_Y = 1;
const int AXIS_RIGHT_STICK_X = 2;
const int AXIS_RIGHT_STICK_Y = 3;
const int AXIS_DPAD = 4;

struct JoystickHidData_t
{
    uint32_t buttons;
    uint8_t axis[5];
};

void joystickHidInit();

bool joystickHidUpdate(JoystickHidData_t *joystickHidData);


#endif // end _JOYSTICK_HID_H_
