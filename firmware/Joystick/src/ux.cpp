#include "ux.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// =============================================================================
// Constants
// =============================================================================

const int UX_REFRESH_RATE = 40;

// Menu input mappings
const int LEFT_MENU_CYCLE_BUTTON = BUTTON_L_BUMPER;
const int RIGHT_MENU_CYCLE_BUTTON = BUTTON_R_BUMPER;
const int MENU_HOME_BUTTON = BUTTON_MENU_1;
const int VISUAL_TEST_BUTTON = BUTTON_MENU_2;
const int VISUAL_TEST_UP_BUTTON = DPAD_UP;
const int VISUAL_TEST_DOWN_BUTTON = DPAD_DOWN;
const int PROPULSION_LEFT_AXIS = AXIS_LEFT_STICK_Y;
const int PROPULSION_RIGHT_AXIS = AXIS_RIGHT_STICK_Y;

// Propulsion system input mappings
const int PROPULSION_CENTER = 128; // Center value for the propulsion system
const int PROPULSION_DEADZONE = 5; // Deadzone for the propulsion system

// Menu top bar constrains
const int MENU_HEADER_HEIGHT = 10;

// Menu navigation
typedef void (*MenuHandler)(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
const int MENU_COUNT = 7;
const int HOME_MENU = 0;

// Menu line positions
const int MENU_LINE_HEIGHT = 9; // Height of each menu line
const int MENU_LINE_1 = 12;
const int MENU_LINE_2 = MENU_LINE_1 + MENU_LINE_HEIGHT;
const int MENU_LINE_3 = MENU_LINE_2 + MENU_LINE_HEIGHT;
const int MENU_LINE_4 = MENU_LINE_3 + MENU_LINE_HEIGHT;
const int CENTER_VERTICALLY = -1;
const int CENTER_VERTICALLY_FULL_SCREEN = -2;

// Visual test values
const int VISUAL_TEST_TOGGLE_HOLD_TIME = 2500; // Time to hold the visual test button before treating it as a toggle instead of a momentary push
const int VISUAL_TEST_TYPE_MIN = 0;
const int VISUAL_TEST_TYPE_MAX = 3;


// =============================================================================
// Global Variables
// =============================================================================

uint32_t lastUxUpdateMillis = 0;
Adafruit_PCD8544 Display = Adafruit_PCD8544(5, 4, 3);
int CurrentMenu = 0;

bool CycleLeftPressed = false;
bool CycleRightPressed = false;

uint32_t VisualTestPressedMillis = 0;
bool VisualTestPressed = false;
bool VisualTestUpPressed = false;
bool VisualTestDownPressed = false;


// =============================================================================
// Function Prototypes
// =============================================================================

// Input detection helpers
void updatePropulsionFromInput(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void updateSelectedMenuFromInput(JoystickHidData_t *joystickHidData);
void updateVisualTestStatusRequestFromInput(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);

// Drawing helpers
void drawMenuTopBar(bool cycleLeftHeld, bool cycleRightHeld);
void drawMenuTitle(String title);
void drawConnectionTimer(SystemStatus_t *systemStatus);
void drawCenteredText(String text, int y);
void drawSelectionTriangle(int x, int y, bool selected);

// String helpers
String getTimeString(uint32_t timeInMillis);

// Menu handlers (render menu and handle user interaction)
void handleStatusMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleVisibilityTestMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleSequenceMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleJoystickMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleIgnitorMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleFaultMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);
void handleArmSystemMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData);

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

bool uxUpdateRequired(bool joystickUpdated, bool comsUpdated)
{
    if (joystickUpdated || comsUpdated)
    {
        return true;
    }
    else if (millis() - lastUxUpdateMillis >= UX_REFRESH_RATE)
    {
        return true;
    }
    else
    {
        // If the joystick was updated, update the display
        return true;
    }
}

void uxUpdate(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    // Clear the display
    Display.clearDisplay();

    // Perform updates to the UX state based on joystick input
    updateSelectedMenuFromInput(joystickHidData);
    updateVisualTestStatusRequestFromInput(systemStatus, joystickHidData);

    // Draw the top menu bar
    drawMenuTopBar(CycleLeftPressed, CycleRightPressed);

    // Draw the current menu and handles any user interaction with the menu
    MenuHandlers[CurrentMenu](systemStatus, joystickHidData);

    // Update the display 
    Display.display();

    // Update the last UX update time
    lastUxUpdateMillis = millis();
}

void updatePropulsionFromInput(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    // Read the left and right propulsion values from the joystick axis
    // If either joystick is in the deadzone, set the propulsion value to the center value
    int leftPropulsion = joystickHidData->axis[PROPULSION_LEFT_AXIS];
    leftPropulsion = leftPropulsion < PROPULSION_CENTER - PROPULSION_DEADZONE || leftPropulsion > PROPULSION_CENTER + PROPULSION_DEADZONE
        ? leftPropulsion : PROPULSION_CENTER;
    int rightPropulsion = joystickHidData->axis[PROPULSION_RIGHT_AXIS];
    rightPropulsion = rightPropulsion < PROPULSION_CENTER - PROPULSION_DEADZONE || rightPropulsion > PROPULSION_CENTER + PROPULSION_DEADZONE
        ? rightPropulsion : PROPULSION_CENTER;

    // Update the propulsion values in the system status
    if (systemStatus->propulsionLeft != leftPropulsion || systemStatus->propulsionRight != rightPropulsion)
    {
        systemStatus->propulsionLeft = leftPropulsion;
        systemStatus->propulsionRight = rightPropulsion;

        // Request an update to the propulsion system
        systemStatus->propulsionUpdateRequested = true;
    }
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

void updateVisualTestStatusRequestFromInput(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    //
    // Visual test button behavior:
    // When the button is pressed, visual test is enabled
    // When the button is held for VISUAL_TEST_TOGGLE_HOLD_TIME, visual test status is not changed (behavior is toggle)
    // When the button is released before VISUAL_TEST_TOGGLE_HOLD_TIME, visual test is disabled (behavior is momentary)
    //

    // When the visual test button is first pressed, update the button status and enable the visual test test
    if (joystickHidData->buttons & VISUAL_TEST_BUTTON && !VisualTestPressed)
    {
        // Update button state
        VisualTestPressed = true;
        VisualTestPressedMillis = millis();

        // If visual test is not already enabled, enable it and request an update
        if (!systemStatus->isVisualTestEnabled)
        {
            systemStatus->isVisualTestEnabled = true;
            systemStatus->visualTestUpdateRequested = true;
        }
    }

    // When the visual test button is released, update the button state and 
    // disable the visual test if the input was a momentary push instead of a toggle
    else if (VisualTestPressed && !(joystickHidData->buttons & VISUAL_TEST_BUTTON))
    {
        // Update button state
        VisualTestPressed = false;

        // If the button is released before the toggle hold time, disable the visual test
        if (millis() - VisualTestPressedMillis < VISUAL_TEST_TOGGLE_HOLD_TIME)
        {
            systemStatus->isVisualTestEnabled = false;
            systemStatus->visualTestUpdateRequested = true;
        }
        
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

void drawConnectionTimer(SystemStatus_t *systemStatus)
{
    // Create text for the connection timer
    String connectionTimerText;
    if (systemStatus->isConnectionLost)
    {
        connectionTimerText = "Reconnecting";
    }
    else
    {
        connectionTimerText = "Connecting";
    }
    connectionTimerText += "\n";
    uint32_t timeSinceLastConnection = millis() - systemStatus->lastMessageReceivedMillis;
    connectionTimerText += getTimeString(timeSinceLastConnection);

    // Draw the connection time in the center of the screen
    drawCenteredText(connectionTimerText, CENTER_VERTICALLY);
}

void drawCenteredText(String text, int y)
{
    // Get the number of lines in the text
    int lineCount = 1;
    int currentTextIndex = 0;
    text.indexOf('\n', 0);
    while ((currentTextIndex = text.indexOf('\n', currentTextIndex)) != -1)
    {
        lineCount++;
        currentTextIndex++;
    }

    // Calculate the y position of the first line
    if (y == CENTER_VERTICALLY)
    {
        y = ((Display.height() - MENU_HEADER_HEIGHT) / 2) - ((lineCount * MENU_LINE_HEIGHT) / 2) + MENU_HEADER_HEIGHT;
    }
    else if (y == CENTER_VERTICALLY_FULL_SCREEN)
    {
        y = (Display.height() / 2) - ((lineCount * MENU_LINE_HEIGHT) / 2);
    }

    // Draw the text
    int lineStartIndex = 0;
    for (int i = 0; i < lineCount; i++)
    {
        // Get the start and end index of the current line
        int lineEndIndex = text.indexOf('\n', lineStartIndex);
        if (lineEndIndex == -1)
        {
            lineEndIndex = text.length();
        }

        // Get the current line of text
        String currentLine = text.substring(lineStartIndex, lineEndIndex);
        int16_t currentLineX, currentLineY;
        uint16_t currentLineWidth, currentLineHeight;
        Display.getTextBounds(currentLine, 0, 0, &currentLineX, &currentLineY, &currentLineWidth, &currentLineHeight);
        Display.setCursor((Display.width() / 2) - (currentLineWidth / 2), y + (i * MENU_LINE_HEIGHT));
        Display.write(currentLine.c_str());

        // Update the start index for the next line
        lineStartIndex = lineEndIndex + 1;
    }
}

void drawSelectionTriangle(int x, int y, bool selected)
{
    // Draw a triangle at the specified position
    const int triangleSize = 7;
    if (selected)
    {
        Display.fillTriangle(x, y, x, y + triangleSize, x + triangleSize, y + (triangleSize / 2), BLACK);
    }
    else
    {
        Display.drawTriangle(x, y, x, y + triangleSize, x + triangleSize, y + (triangleSize / 2), BLACK);
    }
}

String getTimeString(uint32_t timeInMillis)
{
    const int MINUTE_IN_MILLIS = 60000;
    const int SECOND_IN_MILLIS = 1000;
    String timeString = "";
    if (timeInMillis > MINUTE_IN_MILLIS)
    {
        timeString += (timeInMillis / MINUTE_IN_MILLIS);
        timeString += "m";
        timeString += (timeInMillis % MINUTE_IN_MILLIS) / SECOND_IN_MILLIS;
        timeString += "s";
    }
    else
    {
        timeString += timeInMillis / SECOND_IN_MILLIS;
        timeString += "s";
    }
    return timeString;
}


// ==============================================================================
// Menu Handlers
// ==============================================================================

void handleStatusMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    // Draw the title of the system menu
    drawMenuTitle("Status");

    // If the system is not connected to the aggragator, draw the connection timer
    if (!systemStatus->isConnected)
    {
        drawConnectionTimer(systemStatus);
        return;
    }

    // Draw the overall system status on the first line
    // Full caps means that the operator should be aware that fireworks could be ignited
    Display.setCursor(0, MENU_LINE_1);
    if (systemStatus->isSequenceRunning && systemStatus->isSoftwareArmed && systemStatus-> isPhysicallyArmed)
    {
        Display.write("SEQ. RUNNING");
    }
    else if (systemStatus->isSequenceRunning)
    {
        Display.write("Seq. Running");
    }
    else if (systemStatus->isFullyArmed)
    {
        Display.write("SYSTEM ARMED");
    }
    else if (systemStatus->isSoftwareArmed && systemStatus-> isPhysicallyArmed)
    {
        // In this state, the system is armed but not fully armed
        // This means that at least one of the ignitors is either not connected OR is not pyhsically armed
        Display.write("ARMED IN PART");
    }
    else if (systemStatus-> isPhysicallyArmed)
    {
        Display.write("Phys. Armed");
    }
    else if (systemStatus->isSoftwareArmed)
    {
        Display.write("Soft. Armed");
    }
    else
    {
        Display.write("Connected");
    }

    // Draw the current print round trip time on the second line
    Display.setCursor(0, MENU_LINE_2);
    Display.write("Ping:");
    Display.print(systemStatus->lastPingRoundtripMillis);
    Display.write("ms");

    // Draw the ignition system status on the third line
    Display.setCursor(0, MENU_LINE_3);
    Display.write("Ign:");
    if (systemStatus->areAllIgnitorsPhysicallyArmed)
    {
        // All ignitors are connected and armed
        Display.write("Ready");
    }
    else if (systemStatus->areAnyIgnitorsControllersLost)
    {
        // One or more ignitors have lost connection to the aggregator
        Display.write("Lost");
    }
    else if (systemStatus->areAllIgnitorsControllersConnected)
    {
        // All ignitors are connected but not all are armed
        Display.write("Connected");
    }
    else
    {
        Display.write("Waiting");
    }

    // Draw the sequencer status on the fourth line
    Display.setCursor(0, MENU_LINE_4);
    Display.write("Seq:");
    if (systemStatus->isSequenceAborted)
    {
        // The sequencer aborted the sequence
        Display.write("Aborted");
    }
    else if (systemStatus->isSequenceRunning)
    {
        // The sequencer is currently running a sequence
        Display.write("Running");
    }
    else if (systemStatus->isSequencerConnectionLost)
    {
        // The sequencer has lost connection to the aggregator
        Display.write("Lost");
    }
    else if (systemStatus->isSequencerConnected)
    {
        // The sequencer is connected and responding to pings
        Display.write("Ready");
    }
    else
    {
        // The sequencer is not connected
        Display.write("Waiting");
    }
}

void handleVisibilityTestMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    // Draw the title of the visual test menu
    drawMenuTitle("Vis. Test");

    // Update the visual test type based on joystick input
    if (joystickHidData->axis[AXIS_DPAD] == VISUAL_TEST_UP_BUTTON)
    {
        VisualTestUpPressed = true;
    }
    else if (VisualTestUpPressed)
    {
        VisualTestUpPressed = false;
        systemStatus->visualTestType = (systemStatus->visualTestType - 1 + (VISUAL_TEST_TYPE_MAX + 1)) % (VISUAL_TEST_TYPE_MAX + 1);

        // If the visual test is currrently enabled, refresh the request to the new visual test type
        if (systemStatus->isVisualTestEnabled)
        {
            systemStatus->visualTestUpdateRequested = true;
        }
    }
    if (joystickHidData->axis[AXIS_DPAD] == VISUAL_TEST_DOWN_BUTTON)
    {
        VisualTestDownPressed = true;
    }
    else if (VisualTestDownPressed)
    {
        VisualTestDownPressed = false;
        systemStatus->visualTestType = (systemStatus->visualTestType + 1) % (VISUAL_TEST_TYPE_MAX + 1);

        // If the visual test is currrently enabled, refresh the request to the new visual test type
        if (systemStatus->isVisualTestEnabled)
        {
            systemStatus->visualTestUpdateRequested = true;
        }
    }

    // Draw the visual test type selection menu
    // TODO: Consider moving effect names to an array in a header somewhere and drawing menu with a for loop
    {
        drawSelectionTriangle(0, MENU_LINE_1, systemStatus->visualTestType == 0);
        Display.setCursor(12, MENU_LINE_1);
        Display.write("Std. Blink");

        drawSelectionTriangle(0, MENU_LINE_2, systemStatus->visualTestType == 1);
        Display.setCursor(12, MENU_LINE_2);
        Display.write("LED F/B");

        drawSelectionTriangle(0, MENU_LINE_3, systemStatus->visualTestType == 2);
        Display.setCursor(12, MENU_LINE_3);
        Display.write("RGB Wave");

        drawSelectionTriangle(0, MENU_LINE_4, systemStatus->visualTestType == 3);
        Display.setCursor(12, MENU_LINE_4);
        Display.write("LED White");
    }
}

void handleSequenceMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Sequence");

    // If the system is not connected to the aggragator, draw the connection timer
    if (!systemStatus->isConnected)
    {
        drawConnectionTimer(systemStatus);
        return;
    }

    drawCenteredText("Not\nImplemented", CENTER_VERTICALLY);
}

void handleJoystickMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
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

void handleIgnitorMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Ignition");

    // If the system is not connected to the aggragator, draw the connection timer
    if (!systemStatus->isConnected)
    {
        drawConnectionTimer(systemStatus);
        return;
    }

    drawCenteredText("Not\nImplemented", CENTER_VERTICALLY);
}

void handleFaultMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Fault");

    // If there are no faults, draw the "No Faults" message
    if (systemStatus->faultMessageLength == -1)
    {
        drawCenteredText("No Faults", CENTER_VERTICALLY);
        return;
    }

    // If there is a fault, draw the fault message to the display
    Display.setCursor(0, MENU_LINE_1);
    Display.write(systemStatus->faultMessage);
}

void handleArmSystemMenu(SystemStatus_t *systemStatus, JoystickHidData_t *joystickHidData)
{
    drawMenuTitle("Arm System");

    // If the system is not connected to the aggragator, draw the connection timer
    if (!systemStatus->isConnected)
    {
        drawConnectionTimer(systemStatus);
        return;
    }

    drawCenteredText("Not\nImplemented", CENTER_VERTICALLY);
}