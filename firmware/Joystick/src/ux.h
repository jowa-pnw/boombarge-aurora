#ifndef _UX_H_
#define _UX_H_

#include "joystickHid.h"

// Initalize the user experience
void uxInit();

// Update the user experience (update the display output based on user input and current system status)
void uxUpdate(JoystickHidData_t *joystickHidData);


#endif // end _UX_H_