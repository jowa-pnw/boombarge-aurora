#include <Arduino.h>
#include "status.h"
#include "joystickHid.h"
#include "ux.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "JoystickAggragatorComs.pb.h"

SystemStatus_t SystemStatus;

JoystickHidData_t JoystickHidData;

void setup()
{
    joystickHidInit();
    uxInit();
}

void loop()
{
    // Update the status of the joystick HID device
    bool joystickUpdated = joystickHidUpdate(&JoystickHidData);

    // If required, update the UX (user inputs and display outputs) 
    if (uxUpdateRequired(joystickUpdated, false))
    {
        uxUpdate(&SystemStatus, &JoystickHidData);
    }
}
 