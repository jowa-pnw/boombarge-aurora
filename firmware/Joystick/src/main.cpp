#include <Arduino.h>
#include "joystickHid.h"
#include "ux.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "JoystickAggragatorComs.pb.h"

JoystickHidData_t JoystickHidData;

void setup()
{
    while (!Serial); // wait for Arduino Serial Monitor
    Serial.println("\n\nUSB Host Joystick Testing");

    joystickHidInit();
    uxInit();
}

void loop()
{
    bool joystickUpdated = joystickHidUpdate(&JoystickHidData);

    if (joystickUpdated)
    {
        uxUpdate(&JoystickHidData);
    }
}
