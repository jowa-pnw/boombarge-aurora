#include "joystickHid.h"
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
USBHIDParser hid1(myusb);

#define COUNT_JOYSTICKS 4
JoystickController joysticks[COUNT_JOYSTICKS] = {JoystickController(myusb), JoystickController(myusb), JoystickController(myusb), JoystickController(myusb)};
int user_axis[64];
uint32_t buttons_prev = 0;

USBDriver *drivers[] = {&hub1, &joysticks[0], &joysticks[1], &joysticks[2], &joysticks[3], &hid1};
#define CNT_DEVICES (sizeof(drivers) / sizeof(drivers[0]))
const char *driver_names[CNT_DEVICES] = {"Hub1", "joystick[0D]", "joystick[1D]", "joystick[2D]", "joystick[3D]", "HID1"};
bool driver_active[CNT_DEVICES] = {false, false, false, false};

// Lets also look at HID Input devices
USBHIDInput *hiddrivers[] = {&joysticks[0], &joysticks[1], &joysticks[2], &joysticks[3]};
#define CNT_HIDDEVICES (sizeof(hiddrivers) / sizeof(hiddrivers[0]))
const char *hid_driver_names[CNT_DEVICES] = {"joystick[0H]", "joystick[1H]", "joystick[2H]", "joystick[3H]"};
bool hid_driver_active[CNT_DEVICES] = {false};
bool show_changed_only = false;

int psAxis[64];

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16


void PrintDeviceListChanges();



void joystickHidInit()
{
    myusb.begin();
}

bool joystickHidUpdate(JoystickHidData_t *joystickHidData)
{
    myusb.Task();
    PrintDeviceListChanges();

    for (int joystick_index = 0; joystick_index < COUNT_JOYSTICKS; joystick_index++)
    {
        if (joysticks[joystick_index].available())
        {
            uint64_t axis_mask = joysticks[joystick_index].axisMask();
            uint64_t axis_changed_mask = joysticks[joystick_index].axisChangedMask();
            uint32_t buttons = joysticks[joystick_index].getButtons();
            joystickHidData->buttons = buttons;

            Serial.printf("Joystick(%d): buttons = %x", joystick_index, buttons);
            
            if (show_changed_only)
            {
                for (uint8_t i = 0; axis_changed_mask != 0; i++, axis_changed_mask >>= 1)
                {
                    if (axis_changed_mask & 1)
                    {
                        Serial.printf(" %d:%d", i, joysticks[joystick_index].getAxis(i));
                    }
                }
            }
            else
            {
                int axis = 0;
                for (uint8_t i = 0; axis_mask != 0; i++, axis_mask >>= 1)
                {
                    if (axis_mask & 1)
                    {
                        Serial.printf(" %d:%d", i, joysticks[joystick_index].getAxis(i));
                        joystickHidData->axis[axis] = joysticks[joystick_index].getAxis(i);
                        axis++;
                    }
                }
            }
            uint8_t ltv;
            uint8_t rtv;

            for (uint8_t i = 0; i < 64; i++)
            {
                psAxis[i] = joysticks[joystick_index].getAxis(i);
            }

            if (buttons != buttons_prev)
            {
                if (joysticks[joystick_index].joystickType() == JoystickController::PS3)
                {
                    uint8_t leds = 0;
                    if (buttons & 0x8000)
                        leds = 1; // Srq
                    if (buttons & 0x2000)
                        leds = 2; // Cir
                    if (buttons & 0x1000)
                        leds = 4; // Tri
                    if (buttons & 0x4000)
                        leds = 8; // X  //Tri
                    joysticks[joystick_index].setLEDs(leds);
                }
                else
                {
                    uint8_t lr = (buttons & 1) ? 0xff : 0;
                    uint8_t lg = (buttons & 2) ? 0xff : 0;
                    uint8_t lb = (buttons & 4) ? 0xff : 0;
                    joysticks[joystick_index].setLEDs(lr, lg, lb);
                }
                buttons_prev = buttons;
            }

            Serial.println();
            joysticks[joystick_index].joystickDataClear();
        }
    }

    return true;
}



//=============================================================================
// Show when devices are added or removed
//=============================================================================
void PrintDeviceListChanges()
{
    for (uint8_t i = 0; i < CNT_DEVICES; i++)
    {
        if (*drivers[i] != driver_active[i])
        {
            if (driver_active[i])
            {
                Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
                driver_active[i] = false;
            }
            else
            {
                Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
                driver_active[i] = true;

                const uint8_t *psz = drivers[i]->manufacturer();
                if (psz && *psz)
                    Serial.printf("  manufacturer: %s\n", psz);
                psz = drivers[i]->product();
                if (psz && *psz)
                    Serial.printf("  product: %s\n", psz);
                psz = drivers[i]->serialNumber();
                if (psz && *psz)
                    Serial.printf("  Serial: %s\n", psz);
            }
        }
    }

    for (uint8_t i = 0; i < CNT_HIDDEVICES; i++)
    {
        if (*hiddrivers[i] != hid_driver_active[i])
        {
            if (hid_driver_active[i])
            {
                Serial.printf("*** HID Device %s - disconnected ***\n", hid_driver_names[i]);
                hid_driver_active[i] = false;
            }
            else
            {
                Serial.printf("*** HID Device %s %x:%x - connected ***\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
                hid_driver_active[i] = true;

                const uint8_t *psz = hiddrivers[i]->manufacturer();
                if (psz && *psz)
                    Serial.printf("  manufacturer: %s\n", psz);
                psz = hiddrivers[i]->product();
                if (psz && *psz)
                    Serial.printf("  product: %s\n", psz);
                psz = hiddrivers[i]->serialNumber();
                if (psz && *psz)
                    Serial.printf("  Serial: %s\n", psz);
            }
        }
    }
}