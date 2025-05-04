#ifndef _UX_H_
#define _UX_H_

#include "joystickHid.h"
#include "status.h"

// Initalize the user experience
void uxInit();

// Determines if the display needs to be updated based on the joystick, coms data and the internal UX refresh rate
bool uxUpdateRequired(bool joystickUpdated, bool comsUpdated);

// Update the user experience (update the display output based on the current system status and user input)
void uxUpdate(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);


#endif // end _UX_H_