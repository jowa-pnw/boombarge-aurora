using System.IO.Ports;
using BoomBarge.Ignitor;
using Google.Protobuf;

//
// Main Loop
//

uint PingIteration = 1;
SerialPort Serial = SetupSerialPort();

while (true)
{
    // Prompt the user to select an action from the menu
    int action = DrawMenu();

    // If the user selects Exit, close the serial port and exit the program
    if (action == 6)
    {
        Serial.Close();
        Console.WriteLine("Exiting...");
        break;
    }

    // Attempt to perform the selected action
    try
    {
        DoAction(action);
    }
    catch (Exception ex)
    {
        Console.WriteLine($"An exception was thrown: {ex}");
    }
    Console.WriteLine();
}


//
// Functions
// 

SerialPort SetupSerialPort()
{
    // Setup the serial port with the appropriate settings
    SerialPort serialPort = new SerialPort("COM7", 115200, Parity.None, 8, StopBits.One)
    {
        ReadTimeout = 1000,
        WriteTimeout = 1000
    };
    try
    {
        serialPort.Open();
        Console.WriteLine("Serial port opened successfully.");
    }
    catch (Exception ex)
    {
        Console.WriteLine($"Error opening serial port: {ex.Message}");
        throw;
    }
    return serialPort;
}

int DrawMenu()
{
    // Draw a menu that allows the user to select one of these actions: Ping, Arm, Disarm, Get Armed Status, Send Igniton Request
    Console.WriteLine("Select an action:");
    Console.WriteLine(" 1. Ping");
    Console.WriteLine(" 2. Arm");
    Console.WriteLine(" 3. Disarm");
    Console.WriteLine(" 4. Get Armed Status");
    Console.WriteLine(" 5. Send Ignition Request");
    Console.WriteLine(" 6. Exit");
    Console.Write("Enter your choice: ");

    string input = Console.ReadLine() ?? string.Empty;
    if (int.TryParse(input, out int choice) && choice >= 1 && choice <= 6)
    {
        return choice;
    }
    else
    {
        Console.WriteLine("Invalid choice. Please try again.");
        return DrawMenu();
    }
}

void DoAction(int action)
{
    switch (action)
    {
        case 1:
            SendPing();
            break;
        case 2:
            SendArm(true);
            break;
        case 3:
            SendArm(false);
            break;
        case 4:
            GetArmedStatus();
            break;
        case 5:
            SendIgnitionRequest();
            break;
        default:
            Console.WriteLine("Invalid action selected.");
            break;
    }
}

void SendPing()
{
    // Create a ping message
    IgnitorMessage pingMessage = new IgnitorMessage
    {
        GetPing = new GetPing() { Iteration = PingIteration++ }
    };
    byte[] messageBytes = pingMessage.ToByteArray();

    // Enocde the ping message using COBS
    messageBytes = CobsHelper.Encode(messageBytes);

    // Write the encoded message to the serial port
    Serial.Write(messageBytes, 0, messageBytes.Length);

    // Read and decode the reply from the serial port
    byte[] messageReply = ReadPacket();
    messageReply = CobsHelper.Decode(messageReply);

    // Parse the reply message and print the ping iteration
    IgnitorReplyMessage replyMessage = IgnitorReplyMessage.Parser.ParseFrom(messageReply);
    Console.WriteLine($"Got ping reply, iteration: {PingIteration}");
}

void SendArm(bool arm)
{
    // Create an arm message
    IgnitorMessage armMessage = new IgnitorMessage
    {
        SetSystemArmed = new SetSystemArmed() { Armed = arm }
    };
    byte[] messageBytes = armMessage.ToByteArray();

    // Encode the arm message using COBS
    messageBytes = CobsHelper.Encode(messageBytes);

    // Write the encoded message to the serial port
    Serial.Write(messageBytes, 0, messageBytes.Length);
}

void GetArmedStatus()
{
    // Create a get armed status message
    IgnitorMessage getArmedStatusMessage = new IgnitorMessage
    {
        GetSystemArmed = new GetSystemArmed()
    };
    byte[] messageBytes = getArmedStatusMessage.ToByteArray();

    // Encode the get armed status message using COBS
    messageBytes = CobsHelper.Encode(messageBytes);
    
    // Write the encoded message to the serial port
    Serial.Write(messageBytes, 0, messageBytes.Length);
    
    // Read and decode the reply from the serial port
    byte[] messageReply = ReadPacket();
    messageReply = CobsHelper.Decode(messageReply);
    
    // Parse the reply message and print the armed status
    IgnitorReplyMessage replyMessage = IgnitorReplyMessage.Parser.ParseFrom(messageReply);
    Console.WriteLine($"Got armed status: {replyMessage.GetSystemArmedReply.Armed}");
}

void SendIgnitionRequest()
{
    // Prompt the user for the ignitor to trigger
    int ignitorNumber = 0;
    while (true)
    {
        Console.Write("Enter the ignitor number to trigger (0-15): ");
        string input = Console.ReadLine() ?? string.Empty;
        if (!int.TryParse(input, out ignitorNumber) || ignitorNumber < 0 || ignitorNumber > 15)
        {
            Console.WriteLine("Invalid ignitor number. Please enter a number between 0 and 15.");
        }
        break;
    }

    // Create an ignition request message
    IgnitorMessage ignitionRequestMessage = new IgnitorMessage
    {
        RequestIgnition = new IgnitionRequest() { IgnitorId = (uint)ignitorNumber }
    };
    byte[] messageBytes = ignitionRequestMessage.ToByteArray();

    // Encode the ignition request message using COBS
    messageBytes = CobsHelper.Encode(messageBytes);

    // Write the encoded message to the serial port
    Serial.Write(messageBytes, 0, messageBytes.Length);

    // Read and decode the reply from the serial port
    byte[] messageReply = ReadPacket();
    messageReply = CobsHelper.Decode(messageReply);

    // Parse the reply message and print the ignition request status
    IgnitorReplyMessage replyMessage = IgnitorReplyMessage.Parser.ParseFrom(messageReply);
    if (replyMessage.MessageCase == IgnitorReplyMessage.MessageOneofCase.IgnitionConfirmation)
    {
        Console.WriteLine($"Got ignition request reply, ignitor {replyMessage.IgnitionConfirmation.IgnitorId}");
    }
    else
    {
        Console.WriteLine("Got unexpected reply to ignition request.");
    }
}

byte[] ReadPacket()
{
    byte[] readBuffer = new byte[255];
    int readIndex = 0;
    while (true)
    {
        int bytesRead = Serial.Read(readBuffer, readIndex, readBuffer.Length - readIndex);
        readIndex += bytesRead;

        if (readBuffer[readIndex - 1] == 0)
        {
            byte[] packet = new byte[readIndex - 1];
            Array.Copy(readBuffer, 0, packet, 0, readIndex - 1);
            return packet;
        }

    }
}

public class CobsHelper
{
    public static byte[] Encode(byte[] buffer)
    {
        ushort size = (ushort)buffer.Length;
        byte[] encodedBuffer = new byte[size + 2];

        ushort read_index = 0;
        ushort write_index = 1;
        ushort code_index = 0;
        ushort code = 1;

        while (read_index < size)
        {
            if (buffer[read_index] == 0)
            {
                encodedBuffer[code_index] = (byte)code;
                code = 1;
                code_index = write_index++;
                read_index++;
            }
            else
            {
                encodedBuffer[write_index++] = buffer[read_index++];
                code++;

                if (code == 0xFF)
                {
                    encodedBuffer[code_index] = (byte)code;
                    code = 1;
                    code_index = write_index++;
                }
            }
        }

        encodedBuffer[code_index] = (byte)code;

        return encodedBuffer;
    }

    public static byte[] Decode(byte[] encodedBuffer)
    {
        ushort size = (ushort)encodedBuffer.Length;
        byte[] decodedBuffer = new byte[size];

        if (size == 0)
        {
            return decodedBuffer;
        }

        ushort read_index = 0;
        ushort write_index = 0;
        ushort code = 0;
        ushort i = 0;

        while (read_index < size)
        {
            code = encodedBuffer[read_index];

            if (read_index + code > size && code != 1)
            {
                throw new Exception("Unable to decode!");
            }

            read_index++;

            for (i = 1; i < code; i++)
            {
                decodedBuffer[write_index++] = encodedBuffer[read_index++];
            }

            if (code != 0xFF && read_index != size)
            {
                decodedBuffer[write_index++] = (byte)'\0';
            }
        }

        byte[] messageNoTail = new byte[decodedBuffer.Length - 1];
        Array.Copy(decodedBuffer, 0, messageNoTail, 0, decodedBuffer.Length - 1);
        return messageNoTail;
    }
}

public static class MessageExtentions
{
    public static byte[] ToByteArray(this IMessage message) => message.ToByteString().ToArray();
}
