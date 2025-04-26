// Simple test of USB Host Joystick
//
// This example is in the public domain

#include "USBHost_t36.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#include "pb_encode.h"
#include "pb_decode.h"
#include "JoystickAggragatorComs.pb.h"

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!

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

uint8_t joystick_left_trigger_value[COUNT_JOYSTICKS] = {0};
uint8_t joystick_right_trigger_value[COUNT_JOYSTICKS] = {0};
uint64_t joystick_full_notify_mask = (uint64_t)-1;

int psAxis[64];

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16


void PrintDeviceListChanges();
void printAngles();
void getCoords(uint16_t &xc, uint16_t &yc, uint8_t &isTouch);
void getAccel(float &ax, float &ay, float &az);
void getAngles(float &p, float &r);
void getGyro(float &gx, float &gy, float &gz);


static const unsigned char PROGMEM logo16_glcd_bmp[] =
    {
        0B00000000, 0B11000000,
        0B00000001, 0B11000000,
        0B00000001, 0B11000000,
        0B00000011, 0B11100000,
        0B11110011, 0B11100000,
        0B11111110, 0B11111000,
        0B01111110, 0B11111111,
        0B00110011, 0B10011111,
        0B00011111, 0B11111100,
        0B00001101, 0B01110000,
        0B00011011, 0B10100000,
        0B00111111, 0B11100000,
        0B00111111, 0B11110000,
        0B01111100, 0B11110000,
        0B01110000, 0B01110000,
        0B00000000, 0B00110000};

void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h)
{
    uint8_t icons[NUMFLAKES][3];
    randomSeed(666); // whatever seed

    // initialize
    for (uint8_t f = 0; f < NUMFLAKES; f++)
    {
        icons[f][XPOS] = random(display.width());
        icons[f][YPOS] = 0;
        icons[f][DELTAY] = random(5) + 1;

        Serial.print("x: ");
        Serial.print(icons[f][XPOS], DEC);
        Serial.print(" y: ");
        Serial.print(icons[f][YPOS], DEC);
        Serial.print(" dy: ");
        Serial.println(icons[f][DELTAY], DEC);
    }

    while (1)
    {
        // draw each icon
        for (uint8_t f = 0; f < NUMFLAKES; f++)
        {
            display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, BLACK);
        }
        display.display();
        delay(10);

        while (Serial.available())
        {
            switch (Serial.read())
            {
            case 'w':
                display.setContrast(display.getContrast() + 1);
                break;
            case 's':
                if (display.getContrast())
                    display.setContrast(display.getContrast() - 1);
                break;
            case 'e':
                display.setBias(display.getBias() + 1);
                break;
            case 'd':
                if (display.getBias())
                    display.setBias(display.getBias() - 1);
                break;
            case 'R':
                display.setReinitInterval(10);
                break;
            case 'r':
                display.initDisplay();
                display.setReinitInterval(0);
                break;
            }
        }
        Serial.print("contrast (w/s): 0x");
        Serial.print(display.getContrast(), HEX);
        Serial.print("   bias (e/d): 0x");
        Serial.print(display.getBias(), HEX);
        Serial.print("   reinitialize display (r/R): 0x");
        Serial.print(display.getReinitInterval(), HEX);
        Serial.print("   \r");

        // then erase it + move it
        for (uint8_t f = 0; f < NUMFLAKES; f++)
        {
            display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, WHITE);
            // move it
            icons[f][YPOS] += icons[f][DELTAY];
            // if its gone, reinit
            if (icons[f][YPOS] > display.height())
            {
                icons[f][XPOS] = random(display.width());
                icons[f][YPOS] = 0;
                icons[f][DELTAY] = random(5) + 1;
            }
        }
    }
}

void testdrawchar(void)
{
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0, 0);

    for (uint8_t i = 0; i < 168; i++)
    {
        if (i == '\n')
            continue;
        display.write(i);
        // if ((i > 0) && (i % 14 == 0))
        // display.println();
    }
    display.display();
}

void testdrawcircle(void)
{
    for (int16_t i = 0; i < display.height(); i += 2)
    {
        display.drawCircle(display.width() / 2, display.height() / 2, i, BLACK);
        display.display();
    }
}

void testfillrect(void)
{
    uint8_t color = 1;
    for (int16_t i = 0; i < display.height() / 2; i += 3)
    {
        // alternate colors
        display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
        display.display();
        color++;
    }
}

void testdrawtriangle(void)
{
    for (int16_t i = 0; i < min(display.width(), display.height()) / 2; i += 5)
    {
        display.drawTriangle(display.width() / 2, display.height() / 2 - i,
                             display.width() / 2 - i, display.height() / 2 + i,
                             display.width() / 2 + i, display.height() / 2 + i, BLACK);
        display.display();
    }
}

void testfilltriangle(void)
{
    uint8_t color = BLACK;
    for (int16_t i = min(display.width(), display.height()) / 2; i > 0; i -= 5)
    {
        display.fillTriangle(display.width() / 2, display.height() / 2 - i,
                             display.width() / 2 - i, display.height() / 2 + i,
                             display.width() / 2 + i, display.height() / 2 + i, color);
        if (color == WHITE)
            color = BLACK;
        else
            color = WHITE;
        display.display();
    }
}

void testdrawroundrect(void)
{
    for (int16_t i = 0; i < display.height() / 2 - 2; i += 2)
    {
        display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, BLACK);
        display.display();
    }
}

void testfillroundrect(void)
{
    uint8_t color = BLACK;
    for (int16_t i = 0; i < display.height() / 2 - 2; i += 2)
    {
        display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
        if (color == WHITE)
            color = BLACK;
        else
            color = WHITE;
        display.display();
    }
}

void testdrawrect(void)
{
    for (int16_t i = 0; i < display.height() / 2; i += 2)
    {
        display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, BLACK);
        display.display();
    }
}

void testdrawline()
{
    for (int16_t i = 0; i < display.width(); i += 4)
    {
        display.drawLine(0, 0, i, display.height() - 1, BLACK);
        display.display();
    }
    for (int16_t i = 0; i < display.height(); i += 4)
    {
        display.drawLine(0, 0, display.width() - 1, i, BLACK);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (int16_t i = 0; i < display.width(); i += 4)
    {
        display.drawLine(0, display.height() - 1, i, 0, BLACK);
        display.display();
    }
    for (int8_t i = display.height() - 1; i >= 0; i -= 4)
    {
        display.drawLine(0, display.height() - 1, display.width() - 1, i, BLACK);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (int16_t i = display.width() - 1; i >= 0; i -= 4)
    {
        display.drawLine(display.width() - 1, display.height() - 1, i, 0, BLACK);
        display.display();
    }
    for (int16_t i = display.height() - 1; i >= 0; i -= 4)
    {
        display.drawLine(display.width() - 1, display.height() - 1, 0, i, BLACK);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (int16_t i = 0; i < display.height(); i += 4)
    {
        display.drawLine(display.width() - 1, 0, 0, i, BLACK);
        display.display();
    }
    for (int16_t i = 0; i < display.width(); i += 4)
    {
        display.drawLine(display.width() - 1, 0, i, display.height() - 1, BLACK);
        display.display();
    }
    delay(250);
}

//=============================================================================
// Setup
//=============================================================================
void setup()
{
    Serial1.begin(2000000);
    while (!Serial)
        ; // wait for Arduino Serial Monitor
    Serial.println("\n\nUSB Host Joystick Testing");

    display.begin();
    display.setContrast(60);

    myusb.begin();
}

//=============================================================================
// loop
//=============================================================================
void loop()
{
    myusb.Task();
    PrintDeviceListChanges();

    if (Serial.available())
    {
        int ch = Serial.read(); // get the first char.
        while (Serial.read() != -1)
            ;
        if ((ch == 'b') || (ch == 'B'))
        {
            Serial.println("Only notify on Basic Axis changes");
            for (int joystick_index = 0; joystick_index < COUNT_JOYSTICKS; joystick_index++)
                joysticks[joystick_index].axisChangeNotifyMask(0x3ff);
        }
        else if ((ch == 'f') || (ch == 'F'))
        {
            Serial.println("Only notify on Full Axis changes");
            for (int joystick_index = 0; joystick_index < COUNT_JOYSTICKS; joystick_index++)
                joysticks[joystick_index].axisChangeNotifyMask(joystick_full_notify_mask);
        }
        else
        {
            if (show_changed_only)
            {
                show_changed_only = false;
                Serial.println("\n*** Show All fields mode ***");
            }
            else
            {
                show_changed_only = true;
                Serial.println("\n*** Show only changed fields mode ***");
            }
        }
    }

    for (int joystick_index = 0; joystick_index < COUNT_JOYSTICKS; joystick_index++)
    {
        if (joysticks[joystick_index].available())
        {
            uint64_t axis_mask = joysticks[joystick_index].axisMask();
            uint64_t axis_changed_mask = joysticks[joystick_index].axisChangedMask();
            uint32_t buttons = joysticks[joystick_index].getButtons();
            Serial.printf("Joystick(%d): buttons hi = %x", joystick_index, buttons);
            // Serial.printf(" AMasks: %x %x:%x", axis_mask, (uint32_t)(user_axis_mask >> 32), (uint32_t)(user_axis_mask & 0xffffffff));
            // Serial.printf(" M: %lx %lx", axis_mask, joysticks[joystick_index].axisChangedMask());
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
                for (uint8_t i = 0; axis_mask != 0; i++, axis_mask >>= 1)
                {
                    if (axis_mask & 1)
                    {
                        Serial.printf(" %d:%d", i, joysticks[joystick_index].getAxis(i));
                    }
                }
            }
            uint8_t ltv;
            uint8_t rtv;

            for (uint8_t i = 0; i < 64; i++)
            {
                psAxis[i] = joysticks[joystick_index].getAxis(i);
            }

            switch (joysticks[joystick_index].joystickType())
            {
            default:
                break;
            case JoystickController::PS4:
                printAngles();
                ltv = joysticks[joystick_index].getAxis(3);
                rtv = joysticks[joystick_index].getAxis(4);
                if ((ltv != joystick_left_trigger_value[joystick_index]) || (rtv != joystick_right_trigger_value[joystick_index]))
                {
                    joystick_left_trigger_value[joystick_index] = ltv;
                    joystick_right_trigger_value[joystick_index] = rtv;
                    joysticks[joystick_index].setRumble(ltv, rtv);
                }
                break;

            case JoystickController::PS3:
                ltv = joysticks[joystick_index].getAxis(18);
                rtv = joysticks[joystick_index].getAxis(19);
                if ((ltv != joystick_left_trigger_value[joystick_index]) || (rtv != joystick_right_trigger_value[joystick_index]))
                {
                    joystick_left_trigger_value[joystick_index] = ltv;
                    joystick_right_trigger_value[joystick_index] = rtv;
                    joysticks[joystick_index].setRumble(ltv, rtv, 50);
                }
                break;

            case JoystickController::XBOXONE:
            case JoystickController::XBOX360:
                ltv = joysticks[joystick_index].getAxis(3);
                rtv = joysticks[joystick_index].getAxis(4);
                if ((ltv != joystick_left_trigger_value[joystick_index]) || (rtv != joystick_right_trigger_value[joystick_index]))
                {
                    joystick_left_trigger_value[joystick_index] = ltv;
                    joystick_right_trigger_value[joystick_index] = rtv;
                    joysticks[joystick_index].setRumble(ltv, rtv, 50);
                    Serial.printf(" Set Rumble %d %d", ltv, rtv);
                }
                break;
            }
            if (buttons != buttons_prev)
            {
                if (joysticks[joystick_index].joystickType() == JoystickController::PS3)
                {
                    // joysticks[joystick_index].setLEDs((buttons >> 12) & 0xf); //  try to get to TRI/CIR/X/SQuare
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


// PS4 Helpers

float gx, gy, gz;
float ax, ay, az;
float pitch, roll;
uint16_t xc, yc;
uint8_t isTouch;
int16_t xc_old, yc_old;

void printAngles()
{
  // test function calls
  float gx, gy, gz;
  getAccel(ax, ay, az);
  Serial.printf("Accel-g's: %f, %f, %f\n", ax, ay, az);
  getGyro(gx, gy, gz);
  Serial.printf("Gyro-deg/sec: %f, %f, %f\n", gx, gy, gz);

  getAngles(pitch, roll);
  Serial.printf("Pitch/Roll: %f, %f\n", pitch, roll);

  getCoords(xc, yc, isTouch);
}

void getCoords(uint16_t &xc, uint16_t &yc, uint8_t &isTouch)
{

  // uint8_t finger = 0;  //only getting finger 1
  uint8_t Id = 0;

  // Trackpad touch 1: id, active, x, y
  xc = ((psAxis[37] & 0x0f) << 8) | psAxis[36];
  yc = psAxis[38] << 4 | ((psAxis[37] & 0xf0) >> 4),

  isTouch = psAxis[35] >> 7;
  if (xc != xc_old || yc != yc_old)
  {
    Serial.printf("Touch: %d, %d, %d, %d\n", psAxis[33], isTouch, xc, yc);
    xc_old = xc;
    yc_old = yc;
  }
}

void getAccel(float &ax, float &ay, float &az)
{
  int accelx = (int16_t)(psAxis[20] << 8) | psAxis[19];
  int accelz = (int16_t)(psAxis[22] << 8) | psAxis[21];
  int accely = (int16_t)(psAxis[24] << 8) | psAxis[23];

  ax = (float)accelx / 8192;
  ay = (float)accely / 8192;
  az = (float)accelz / 8192;
}

void getAngles(float &p, float &r)
{
  getAccel(ax, ay, az);
  p = (atan2f(ay, az) + PI) * RAD_TO_DEG;
  r = (atan2f(ax, az) + PI) * RAD_TO_DEG;
}

void getGyro(float &gx, float &gy, float &gz)
{
  int gyroy = (int16_t)(psAxis[14] << 8) | psAxis[13];
  int gyroz = (int16_t)(psAxis[16] << 8) | psAxis[15];
  int gyrox = (int16_t)(psAxis[18] << 8) | psAxis[17];

  gx = (float)gyrox * RAD_TO_DEG / 1024;
  gy = (float)gyroy * RAD_TO_DEG / 1024;
  gz = (float)gyroz * RAD_TO_DEG / 1024;
}