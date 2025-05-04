#ifndef _STATUS_H_
#define _STATUS_H_

#include <Arduino.h>

const int IGNIOR_CONTROLLER_COUNT = 3;

enum UxNotificationType
{
    UX_NOTIFICATION_NONE = 0,
    UX_NOTIFICATION_CONNECTED,
    UX_NOTIFICATION_CONNECTION_LOST,
    UX_NOTIFICATION_ARMED,
    UX_NOTIFICATION_DISARMED,
    UX_NOTIFICATION_SEQUENCE_TRIGGERED,
    UX_NOTIFICATION_SEQUENCE_ABORTED,
    UX_NOTIFICATION_FAULT,
};


// The status of the system (joystick + aggregator)
struct SystemStatus_t
{
    //
    // UX Requests
    // 

    // Wether or not arming the system has been requested by the UX
    bool softwareArmRequested;

    // Whether or not triggering the selected sequence has been requested by the UX
    bool sequenceTriggerRequested;

    // Whether or not aborting the selected sequence has been requested by the UX
    bool sequenceAbortRequested;

    // Whether or not a propulsion system update has been requested by the UX
    bool propulsionUpdateRequested;

    // Whether or not a visual test has been requested by the UX
    bool requestVisualTestToggle;


    //
    // UX Noitification
    //

    // When the last notification was sent to the UX
    uint32_t notificationStartMillis;

    // The type of the last notification sent to the UX
    UxNotificationType notificationType;


    //
    // Aggregator connection status
    //

    // Whether or not the conntroller is connected to the aggregator
    bool isConnected;

    // Set to true only if there was previously a connection to the aggregator and the connection was lost
    bool isConnectionLost; 

    // The last time a parsable reply was received from the aggregator
    uint32_t lastMessageReceivedMillis; 


    //
    // Ping status
    //

    // The ping iteration of the last ping that was sent to the aggregator
    uint32_t pingItteration;

    // The last time a ping was sent to the aggregator
    uint32_t lastPingSentMillis;

    // The last time a ping was received from the aggregator
    uint32_t lastPingRoundtripMillis;


    //
    // Ignitor status
    //

    // Whether or not all arm/disarm flags are set to armed
    // This also means that all ignitor controllers are connected and responding to pings
    bool isFullyArmed;

    // Whether or not all ignitor controllers are connected and responding to pings
    bool areAllIgnitorsControllersConnected;

    // Whether or not any ignitor controllers have lost connection to the aggregator
    bool areAnyIgnitorsControllersLost;

    // Whether or not all ignitors are physically armed
    bool areAllIgnitorsPhysicallyArmed;

    // Whether or not the aggregator is "software" armed
    bool isSoftwareArmed;

    // Whether or not the aggregator detects that the physical arm/disarm switch is in the armed position
    bool isPhysicallyArmed;

    // Whether or not the ignitor controllers are connected and responding to pings
    bool ignitorControllersConnected[IGNIOR_CONTROLLER_COUNT];

    // Whether or not the ignitor controllers have lost connection to the aggregator
    bool ignitorControllersLostConnection[IGNIOR_CONTROLLER_COUNT];

    // Wether or not the ignitor controllers detect that the physical arm/disarm switch is in the armed position
    bool ignitorControllersPhysicallyArmed[IGNIOR_CONTROLLER_COUNT];


    //
    // Sequencer status
    //

    // Whether or not the sequencer is connected and responding to pings
    bool isSequencerConnected;

    // Whether or not the sequencer has lost connection to the aggregator
    bool isSequencerConnectionLost;

    // Whether or not a sequence is currently running
    bool isSequenceRunning;

    // Whether or not the sequencer has aborted the sequence
    bool isSequenceAborted;

    // The number of seuences the sequence controller has available for playback
    uint32_t sequenceCount;

    // The index of the currently selected sequence / running sequence
    uint32_t sequenceId; 

    // If the sequence is running, the current frame of the sequence
    // If the sequence is not running, the last frame of the sequence that was played or the frame to start playing from
    uint32_t sequenceFrame;

    // The total number of frames in the currently running sequence
    uint32_t sequenceFrameCount;
    

    //
    // Propulsion status 
    //

    /// The last time a propulsion system update was sent to the aggregator
    uint32_t lastPropulsionMessageSentMillis;

    // The left propulsion value
    uint8_t propulsionLeft;
    
    // The right propulsion value
    uint8_t propulsionRight;


    //
    // Visual test status
    //

    // Whether or not the visual test is enabled
    bool isVisualTestEnabled;

    // The type of visual test that is currently selected or running
    uint8_t visualTestType;


    //
    // Fault status
    //

    // The last fault message received from the aggregator
    char faultMessage[255];

    // The length of the last fault message received from the aggregator
    int faultMessageLength = -1;
};




#endif // end _STATUS_H_