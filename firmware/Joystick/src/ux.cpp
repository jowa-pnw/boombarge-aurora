#include "ux.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// =============================================================================
// Constants
// =============================================================================

// Menu input mappings
const int LEFT_MENU_CYCLE_BUTTON = BUTTON_L_BUMPER;
const int RIGHT_MENU_CYCLE_BUTTON = BUTTON_R_BUMPER;
const int MENU_HOME_BUTTON = BUTTON_MENU_1;

// Menu top bar constrains
const int MENU_HEADER_HEIGHT = 10;

// Menu navigation
typedef void (*MenuHandler)(JoystickHidData_t *joystickHidData);
const int MENU_COUNT = 7;
const int HOME_MENU = 0;

// Menu line positions
const int MENU_LINE_1 = 12;
const int MENU_LINE_2 = 21;
const int MENU_LINE_3 = 30;
const int MENU_LINE_4 = 39;


// =============================================================================
// Global Variables
// =============================================================================

Adafruit_PCD8544 Display = Adafruit_PCD8544(5, 4, 3);
int CurrentMenu = 0;
bool CycleLeftPressed = false;
bool CycleRightPressed = false;


// =============================================================================
// Function Prototypes
// =============================================================================

// Input detection helpers
void updateSelectedMenuFromInput(JoystickHidData_t *joystickHidData);

// Drawing helpers
void drawMenuTopBar(bool cycleLeftHeld, bool cycleRightHeld);
void drawMenuTitle(String title);

// Menu handlers (render menu and handle user interaction)
void handleStatusMenu(JoystickHidData_t *joystickHidData);
void handleVisibilityTestMenu(JoystickHidData_t *joystickHidData);
void handleSequenceMenu(JoystickHidData_t *joystickHidData);
void handleJoystickMenu(JoystickHidData_t *joystickHidData);
void handleIgnitorMenu(JoystickHidData_t *joystickHidData);
void handleFaultMenu(JoystickHidData_t *joystickHidData);
void handleArmSystemMenu(JoystickHidData_t *joystickHidData);

// Menu handler function pointers
const MenuHandler MenuHandlers[MENU_COUNT] = 
{
    handleStatusMenu,
    handleVisibilityTestMenu,
    handleSequenceMenu,
    handleJoystickMenu,
    handleIgnitorMenu,
    handleFaultMenu,
    handleArmSystemMenu,
};

// =============================================================================
// Function Implementations
// =============================================================================

void uxInit()
{
    // Start the display
    Display.begin();
    Display.setContrast(60);

    // Clear the Adafruit logo from the display
    Display.clearDisplay();

    // Set the default text size and color
    Display.setTextSize(1);
    Display.setTextColor(BLACK);

    // Draw the top menu bar
    drawMenuTopBar(CycleLeftPressed, CycleRightPressed);
    drawMenuTitle("Status");

    // Render the current buffer to the display
    Display.display();
}

void uxUpdate(JoystickHidData_t *joystickHidData)
{
    // Clear the display
    Display.clearDisplay();

    // Update which menu is selected based on joystick input data
    updateSelectedMenuFromInput(joystickHidData);

    // Draw the top menu bar
    drawMenuTopBar(CycleLeftPressed, CycleRightPressed);

    // Draw the current menu and handles any user interaction with the menu
    MenuHandlers[CurrentMenu](joystickHidData);

    // Update the display 
    Display.display();
}

void updateSelectedMenuFromInput(JoystickHidData_t *joystickHidData)
{
    // If the home button is pressed, reset the menu to the home menu
    if (joystickHidData->buttons & MENU_HOME_BUTTON)
    {
        CurrentMenu = HOME_MENU;
    }

    // Cycle the selected menu left whenever the left menu cycle button is released
    if (joystickHidData->buttons & LEFT_MENU_CYCLE_BUTTON)
    {
        CycleLeftPressed = true;
    }
    else if (CycleLeftPressed)
    {
        CycleLeftPressed = false;
        CurrentMenu = (CurrentMenu - 1 + MENU_COUNT) % MENU_COUNT;
    }

    // Cycle the selected menu right whenever the right menu cycle button is released
    if (joystickHidData->buttons & RIGHT_MENU_CYCLE_BUTTON)
    {
        CycleRightPressed = true;
    }
    else if (CycleRightPressed)
    {
        CycleRightPressed = false;
        CurrentMenu = (CurrentMenu + 1) % MENU_COUNT;
    }
}

void drawMenuTopBar(bool cycleLeftPressed, bool cycleRightPressed)
{
    const int triangleSize = 8;
    const int leftTriangleStartX = Display.width() - 1 - triangleSize;

    // Draw left navigation triangle
    // Invert the triangle region if the left menu cycle button is currently pressed
    int leftTriangleColor = BLACK;
    if (cycleLeftPressed)
    {
        Display.fillRect(0, 0, triangleSize, triangleSize + 1, BLACK);
        leftTriangleColor = WHITE;
    }
    Display.fillTriangle(0, triangleSize / 2, triangleSize, 0, triangleSize, triangleSize, leftTriangleColor);

    // Draw right navigation triangle
    // Invert the triangle region if the right menu cycle button is currently pressed
    int rightTriangleColor = BLACK;
    if (cycleRightPressed)
    {
        Display.fillRect(leftTriangleStartX, 0, triangleSize, triangleSize + 1, BLACK);
        rightTriangleColor = WHITE;
    }
    Display.fillTriangle(Display.width() - 1, triangleSize / 2, leftTriangleStartX, 0, leftTriangleStartX, triangleSize, rightTriangleColor);

    // Draw buttom of menu bar
    Display.drawLine(0, MENU_HEADER_HEIGHT, Display.width(), MENU_HEADER_HEIGHT, BLACK);
}

void drawMenuTitle(String title)
{
    // Draw the title into the top menu bar
    int16_t titleX, titleY;
    uint16_t titleWidth, titleHeight;
    Display.getTextBounds(title, 0, 0, &titleX, &titleY, &titleWidth, &titleHeight);
    Display.setCursor((Display.width()/2) - (titleWidth/2), 1);
    Display.write(title.c_str());
}

void handleStatusMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Status");

    Display.setCursor(0, MENU_LINE_1);
    Display.write("Test 1");

    Display.setCursor(0, MENU_LINE_2);
    Display.write("Test 2");

    Display.setCursor(0, MENU_LINE_3);
    Display.write("Test 3");

    Display.setCursor(0, MENU_LINE_4);
    Display.write("Test 4");
}

void handleVisibilityTestMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Vis. Test");
}

void handleSequenceMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Sequence");
}

void handleJoystickMenu(JoystickHidData_t *joystickHidData)
{
    // Draw the title into the top menu bar
    drawMenuTitle("Joysticks");

    // Draw the left and right joystick axis values
    Display.setCursor(0, MENU_LINE_1);
    Display.write("Lx: ");
    Display.print(joystickHidData->axis[AXIS_LEFT_STICK_X]);
    Display.setCursor(0, MENU_LINE_2);
    Display.write("Ly: ");
    Display.print(joystickHidData->axis[AXIS_LEFT_STICK_Y]);
    Display.setCursor(0, MENU_LINE_3);
    Display.write("Rx: ");
    Display.print(joystickHidData->axis[AXIS_RIGHT_STICK_X]);
    Display.setCursor(0, MENU_LINE_4);
    Display.write("Ry: ");
    Display.print(joystickHidData->axis[AXIS_RIGHT_STICK_Y]);
}

void handleIgnitorMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Ignitor");
}

void handleFaultMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Fault");
}

void handleArmSystemMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Arm System");
}