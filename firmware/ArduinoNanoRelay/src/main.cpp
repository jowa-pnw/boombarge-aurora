#include <Arduino.h>
#include <PacketSerial.h>
#include "pb_encode.h"
#include "pb_decode.h"
#include "Ignitor.pb.h"


#define RELAY_OPEN LOW
#define RELAY_CLOSE HIGH

void OpenTimedOutRelays();
void OnPacketReceived(const uint8_t* buffer, size_t size);
void SendReply(const IgnitorReplyMessage& reply, const pb_msgdesc_t *fields);

// The number of relays being controlled by this board
const uint8_t RELAY_COUNT = 16;

// The pin mappings for all of the relays
const uint8_t RELAY_PINS[RELAY_COUNT] =
{
    2,  // Relay 00
    3,  // Relay 01
    4,  // Relay 02
    5,  // Relay 03
    6,  // Relay 04
    7,  // Relay 05
    8,  // Relay 06
    9,  // Relay 07
    
    A0, // Relay 08
    A1, // Relay 09
    A2, // Relay 10
    A3, // Relay 11
    A4, // Relay 12
    A5, // Relay 13
    10, // Relay 14
    11, // Relay 15
};

const uint16_t RelayClosePeriod = 700; // The amount of time to keep a relay closed before opening it again

// The baud rate for communication with the master
const uint32_t SERIAL_BAUD_RATE = 115200;

// The state of each relay
bool RelayStates[RELAY_COUNT] = {RELAY_OPEN};

// Records the time that each releay should be opened again
uint32_t RelayCloseTimeoutMillis[RELAY_COUNT] = {0};

bool SystemArmed = false; // Tracks if the system is armed


PacketSerial PSerial;


void setup()
{
    // Setup all of the relays this board is controlling
    for (int currentRelay = 0; currentRelay < RELAY_COUNT; currentRelay++)
    {
        // Prewrite the output of the digital outputs to high so we don't incidentally
        // close the relays when pin mode is set.
        digitalWrite(RELAY_PINS[currentRelay], RELAY_OPEN);

        // Set the current relay control pin as an output
        pinMode(RELAY_PINS[currentRelay], OUTPUT);
    }

    // Start the seial connection with the master
    PSerial.begin(SERIAL_BAUD_RATE);
    PSerial.setPacketHandler(&OnPacketReceived);
}

void loop()
{
    // Open any relays that have timed out
    OpenTimedOutRelays();

    // Update the PacketSerial instance to process incoming packets
    PSerial.update();
}

void OpenTimedOutRelays()
{
    // Open any relays that have timed out
    for (int currentRelay = 0; currentRelay < RELAY_COUNT; currentRelay++)
    {
        // If the relay is currently closed and the timeout has passed, open it
        if (RelayStates[currentRelay] == RELAY_CLOSE && millis() >= RelayCloseTimeoutMillis[currentRelay])
        {
            // Open the relay
            digitalWrite(RELAY_PINS[currentRelay], RELAY_OPEN);
            RelayStates[currentRelay] = RELAY_OPEN;
        }
    }
}

void OnPacketReceived(const uint8_t* buffer, size_t size)
{
    // Attempt to decode the received packet
    IgnitorMessage request = IgnitorMessage_init_zero;
    pb_istream_t decodeStream = pb_istream_from_buffer(buffer, size);
    bool decodeSuccess = pb_decode(&decodeStream, IgnitorMessage_fields, &request);

    // If the decoding was unsuccessful, return
    if (!decodeSuccess)
    {
        return;
    }

    // Handle the different types of messages
    switch (request.which_message)
    {
        case IgnitorMessage_get_ping_tag:
        {
            // Create a response message
            IgnitorReplyMessage response = IgnitorReplyMessage_init_zero;
            response.which_message = IgnitorReplyMessage_ping_reply_tag;
            response.message.ping_reply.iteration = request.message.get_ping.iteration;

            // Send the response message
            SendReply(response, IgnitorReplyMessage_fields);
            break;
        }

        case IgnitorMessage_get_system_armed_tag:
        {
            // Create a response message
            IgnitorReplyMessage response = IgnitorReplyMessage_init_zero;
            response.which_message = IgnitorReplyMessage_get_system_armed_reply_tag;
            response.message.get_system_armed_reply.armed = SystemArmed;

            // Send the response message
            SendReply(response, IgnitorReplyMessage_fields);
            break;
        }

        case IgnitorMessage_set_system_armed_tag:
        {
            // Set the system armed state based on the received message
            SystemArmed = request.message.set_system_armed.armed;
            break;
        }

        case IgnitorMessage_request_ignition_tag:
        {
            // If the system is not armed, ignore the ignite command
            if (!SystemArmed)
            {
                return;
            }

            // Get the relay index from the ignite message
            uint8_t relayIndex = request.message.request_ignition.ignitor_id;

            // If the relay index is valid, close the relay
            if (relayIndex < RELAY_COUNT)
            {
                digitalWrite(RELAY_PINS[relayIndex], RELAY_CLOSE);
                RelayStates[relayIndex] = RELAY_CLOSE;
                RelayCloseTimeoutMillis[relayIndex] = millis() + RelayClosePeriod;
            }

            // Create a response message
            IgnitorReplyMessage response = IgnitorReplyMessage_init_zero;
            response.which_message = IgnitorReplyMessage_ignition_confirmation_tag;
            response.message.ignition_confirmation.ignitor_id = request.message.request_ignition.ignitor_id;

            // Send the response message
            SendReply(response, IgnitorReplyMessage_fields);
            break;
        }

        default:
        {
            // Unknown message type; ignore it
            break;
        }
    }
}

void SendReply(const IgnitorReplyMessage& reply, const pb_msgdesc_t *fields)
{
    // Prepare the buffer and stream for the reply
    uint8_t replyBuffer[255];
    pb_ostream_t encodeStream = pb_ostream_from_buffer(replyBuffer, sizeof(replyBuffer));
    
    // Encode and send the reply message
    if (pb_encode(&encodeStream, fields, &reply))
    {
        PSerial.send(replyBuffer, encodeStream.bytes_written);
    }
}
