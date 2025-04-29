#include "ux.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// =============================================================================
// Constants
// =============================================================================

const int MENU_HEADER_HEIGHT = 10;

typedef void (*DrawMenuFunction)(JoystickHidData_t *joystickHidData);
const int NUM_MENU_FUNCTIONS = 7;

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

void drawMenuTopBar(bool cycleLeftHeld, bool cycleRightHeld);
void drawMenuTitle(String title);
void drawStatusMenu(JoystickHidData_t *joystickHidData);
void drawVisibilityTestMenu(JoystickHidData_t *joystickHidData);
void drawSequenceMenu(JoystickHidData_t *joystickHidData);
void drawJoystickMenu(JoystickHidData_t *joystickHidData);
void drawIgnitorMenu(JoystickHidData_t *joystickHidData);
void drawFaultMenu(JoystickHidData_t *joystickHidData);
void drawArmSystemMenu(JoystickHidData_t *joystickHidData);

const DrawMenuFunction MenuFunctions[NUM_MENU_FUNCTIONS] = 
{
    drawStatusMenu,
    drawVisibilityTestMenu,
    drawSequenceMenu,
    drawJoystickMenu,
    drawIgnitorMenu,
    drawFaultMenu,
    drawArmSystemMenu,
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
    if (joystickHidData->buttons & BUTTON_L_BUMPER)
    {
        CycleLeftPressed = true;
    }
    else if (CycleLeftPressed)
    {
        CycleLeftPressed = false;
        CurrentMenu = (CurrentMenu - 1 + NUM_MENU_FUNCTIONS) % NUM_MENU_FUNCTIONS;
    }

    if (joystickHidData->buttons & BUTTON_R_BUMPER)
    {
        CycleRightPressed = true;
    }
    else if (CycleRightPressed)
    {
        CycleRightPressed = false;
        CurrentMenu = (CurrentMenu + 1) % NUM_MENU_FUNCTIONS;
    }

    // Clear the display
    Display.clearDisplay();

    // Draw the top menu bar
    drawMenuTopBar(CycleLeftPressed, CycleRightPressed);

    // Draw the current menu
    if (CurrentMenu >= 0 && CurrentMenu < NUM_MENU_FUNCTIONS)
    {
        MenuFunctions[CurrentMenu](joystickHidData);
    }

    Display.display();
}

void drawMenuTopBar(bool cycleLeftPressed, bool cycleRightPressed)
{
    const int triangleSize = 8;
    const int leftTriangleStartX = Display.width() - 1 - triangleSize;

    // Draw left navigation triangle
    int leftTriangleColor = BLACK;
    if (cycleLeftPressed)
    {
        Display.fillRect(0, 0, triangleSize, triangleSize + 1, BLACK);
        leftTriangleColor = WHITE;
    }
    Display.fillTriangle(0, triangleSize / 2, triangleSize, 0, triangleSize, triangleSize, leftTriangleColor);

    // Draw right navigation triangle
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

void drawStatusMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Status");

    Display.setCursor(0, 12);
    Display.write("Test 1");

    Display.setCursor(0, 21);
    Display.write("Test 2");

    Display.setCursor(0, 30);
    Display.write("Test 3");

    Display.setCursor(0, 39);
    Display.write("Test 4");
}

void drawVisibilityTestMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Vis. Test");
}

void drawSequenceMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Sequence");
}

void drawJoystickMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Joystick");
}

void drawIgnitorMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Ignitor");
}

void drawFaultMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Fault");
}

void drawArmSystemMenu(JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Arm System");
}