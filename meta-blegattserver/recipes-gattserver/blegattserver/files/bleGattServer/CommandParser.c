
/*
 * @file    CommandParser.c
 * @author  Nestle Firmware Team
 * @version V1.0.0
 * @date
 * @brief   Source file for BLE App parser interface
 */

/* Standard Includes --------------------------------------------------------*/

#include "CommandParser.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Project Includes ---------------------------------------------------------*/

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

#define MAX_SENSORS       7
#define MAX_MESSAGE_SIZE 100


/*Commands between BLE Hook to Harvest main Hook*/
#define HARVEST_ENABLE_DISABLE_CMD         (0x01)
#define LOADCELL_TARE_SCALE_CMD            (0x02)//Tare the scale
#define LOADCELL_CALIB_WEIGHT_CMD          (0x03)
#define SET_WIFI_CREDEN_CMD                (0x04)
#define GET_WIFI_CREDEN_CMD                (0x05)
#define A55_SHUTDOWN_CMD                   (0x06)
#define BOWL_RESTART_CMD                   (0x07)
#define GET_SESSION_DETAILS_CMD            (0x08)
#define GET_SESSION_DATA_CMD               (0x09)
#define GET_SESSION_DETAILS_ACTIVITY_CMD   (0x0A)
#define GET_SESSION_DATA_ACTIVITY_CMD      (0x0B)
#define GET_TOTALSESSION_CNT_CMD           (0x0C)
#define SET_SENSOR_BOWL_CMD                (0x0D)

/* Private macro ------------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/

extern int msgid;

/* Private variables --------------------------------------------------------*/

const char *sensors[MAX_SENSORS] = {"Ambient Sensor", "IR Sensor", "Load Cell", "MIC", "IMU Sensor", "Camera 1", "Camera 2"};

struct message
{
    long message_type;
    uint8_t message_text[MAX_MESSAGE_SIZE];
};

struct message msg;

/* Private function prototypes -----------------------------------------------*/

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
const char **decodeSelection(int number, int *selectedCount)
{
    const char **selectedSensors = (const char **)malloc(MAX_SENSORS * sizeof(const char *));
    if (!selectedSensors)
    {
        printf("\n BLE_HOOK_INFO :Memory allocation failed\n");
        exit(1);
    }
    int i, count = 0;
    for (i = 0; i < MAX_SENSORS; i++)
    {
        if ((number >> i) & 1)
        {
            selectedSensors[count++] = sensors[i];
        }
    }
    *selectedCount = count;
    return selectedSensors;
}

// Dummy session data (assuming it's a global variable)
uint8_t sessionData[5][27] = {
    {0xAB, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x12, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x20, 0xd8, 0x3b, 0x3e, 0x87, 0xbc, 0x38, 0x7e, 0x72},
    {0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x12, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x25, 0x1e, 0x6d, 0xba, 0x44, 0x54, 0x4d, 0x9b, 0xaf},
    {0xDE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x12, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x35, 0x9e, 0x43, 0x6f, 0xd5, 0x95, 0x07, 0xbc, 0x6a},
    {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x12, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x45, 0x42, 0xb0, 0x3a, 0x09, 0x89, 0x95, 0x79, 0xee},
    {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x12, 0x00, 0x00, 0x00, 0x00, 0x65, 0xE4, 0x86, 0x60, 0x0f, 0xf9, 0x94, 0x0b, 0x13, 0x7c, 0x2b, 0xe7}};

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void connectToWifi(const char *ssid, const char *password)
{
    printf("\n BLE_HOOK_INFO :Connecting to WiFi...\n");
    printf("\n BLE_HOOK_INFO :SSID: %s\n", ssid);
    printf("\n BLE_HOOK_INFO :Password: %s\n", password);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

int isWiFiConnected()
{
    return 1; // Assuming WiFi is always connected in this dummy implementation
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void parseCommand(const uint8_t *data, int data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t))
{
    printf("\n BLE_HOOK_INFO :BLE Command Received from mobile app\n");

    for (int i = 0; i < data_length; i++)
    {
        printf("%d ", data[i]); // Print the element
    }
    printf("\n");

    if (data_length == 0)
    {
        printf("\n BLE_HOOK_INFO :Received empty data.\n");
        return;
    }
    if (data[0] != 0x01)
    { // Start of header check
        printf("\n BLE_HOOK_INFO :Invalid start of header.\n");
        return;
    }
    if (data_length < 2 || data_length != data[1])
    {
        printf("\n BLE_HOOK_INFO :Data length mismatch.\n");
        return;
    }

    uint8_t cmd = data[3]; // CMD
    switch (cmd)
    {
    case 0x10:

        msg.message_type    = 1;                            // Message type can be used to differentiate messages if needed
        msg.message_text[0] = HARVEST_ENABLE_DISABLE_CMD;    // First byte is the command ID
        msg.message_text[1] = data[4]; // Start /Stop
     
        // sending message to harvest main blehook
        if (msgsnd(msgid, &msg, sizeof(msg.message_text), 0) == -1)
        {
            printf("\n BLE_HOOK_INFO :Error, Not able to send harvest start/stop command to Harvest main.\n");
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n BLE_HOOK_INFO : send harvest start/stop command  sent to Harvest main = %s\n", msg.message_text);
            // for (int i = 0; i < (int)sizeof(msg.message_text); i++)
            // {
            // printf("\n BLE_HOOK_INFO :%d ", msg.message_text[i]); // Print the element
            // }
            printf("\n");
        }

        startOrStopRecording(data, data_length, readCallback, notifyCallback);

        break;
    case 0x05:
        getTimeSync(data, data_length, readCallback);
        break;
    case 0x06:
        setTimeSync(data, data_length, readCallback);
        break;
    case 0x1C:
        setWifiConfig(data, data_length, readCallback);
        break;
    case 0x1D:
        getWifiConfig(data, data_length, readCallback);
        break;
    case 0x16:
        getDashboardInfoBowl(data, data_length, readCallback);
        break;
    case 0x1A:
        setDogSize(data, data_length, readCallback);
        break;
    case 0x11:
        ReadRecordingSessionDetails(data, data_length, readCallback);
        break;
    case 0x12:
        ReadRecordingSessionData(data, data_length, readCallback);
        break;
    case 0x13:
        ReadRecordingSessionDetailsActivity(data, data_length, readCallback);
        break;
    case 0x14:
        ReadRecordingSessionDataActivity(data, data_length, readCallback);
        break;
    case 0x1E:
        getTotalSessionsCount(data, data_length, readCallback);
        break;
    case 0x1F:
        getSessionByIndex(data, data_length, readCallback);
        break;
    case 0x20:
        setSensorForBowl(data, data_length, readCallback);
        break;

    case 0x21: // load cell tare

        msg.message_type    = 1;       // Message type can be used to differentiate messages if needed
        msg.message_text[0] = LOADCELL_TARE_SCALE_CMD; // First byte is the command ID
        msg.message_text[1] = 0;

        if (msgsnd(msgid, &msg, sizeof(msg.message_text), 0) == -1)
        {
            printf("\n BLE_HOOK_INFO :Error, Not able to send load cell tare command to Harvest main.\n");
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n BLE_HOOK_INFO : send load cell tare command  sent to Harvest main = %s\n", msg.message_text);
            printf("\n");
        }

        break;

    case 0x22: // load cell start calibrate

        msg.message_type    = 1;                            // Message type can be used to differentiate messages if needed
        msg.message_text[0] = LOADCELL_CALIB_WEIGHT_CMD;    // First byte is the command ID
        msg.message_text[1] = data[4]; // MSB
        msg.message_text[2] = data[5]; // LSB

        if (msgsnd(msgid, &msg, sizeof(msg.message_text), 0) == -1)
        {
            printf("\n BLE_HOOK_INFO :Error, Not able to send harvest start/stop command to Harvest main.\n");
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n BLE_HOOK_INFO : send harvest start/stop command  sent to Harvest main = %s\n", msg.message_text);
            printf("\n");
        }

        break;
    default:
        printf("\n BLE_HOOK_INFO :Unknown or unsupported command.\n");
        break;
    }
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void startOrStopRecording(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t))
{
    bool startStop = data[4] == 0x01; // Start/Stop flag

    uint8_t additionalData[1] = {0x00}; // Additional data for the acknowledgment

    if (startStop)
    {
        sendAck(0x10, additionalData, sizeof(additionalData), readCallback);
        printf("\n BLE_HOOK_INFO :A55 CORE .Start Harvesting command received from BLE App\n");
    }
    else
    {
        sendAck(0x10, additionalData, sizeof(additionalData), readCallback);
        printf("\n BLE_HOOK_INFO :A55 CORE .Stop Harvesting command received from BLE App\n");
    }
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void setWifiConfig(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    char wifiSSID[21] = {0}; // 20 characters + null terminator
    size_t ssidLen = data[4];
    size_t len1 = 5 + (20 - ssidLen);
    for (size_t i = len1; i <= 24; ++i)
    {
        wifiSSID[i - len1] = (char)data[i];
    }

    char wifiPassword[21] = {0}; // 20 characters + null terminator
    size_t passLen = data[25];

    size_t len2 = 26 + (20 - passLen);

    for (size_t i = len2; i <= 45; ++i)
    {
        wifiPassword[i - len2] = (char)data[i];
        // printf("Index for pass: %d, Value: %c\n", i - len2, wifiPassword[i - len2]);
    }

    // Print SSID and password
    printf("\n BLE_HOOK_INFO :WiFi SSID: %s\n", wifiSSID);
    printf("\n BLE_HOOK_INFO :WiFi Password: %s\n", wifiPassword);

    connectToWifi(wifiSSID, wifiPassword);
    printf("\n BLE_HOOK_INFO :Status: %d\n", isWiFiConnected());

    uint8_t additionalData[1] = {0x00}; // Additional data for the acknowledgment
    sendAck(0x1C, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void getWifiConfig(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t additionalData[43] = {0}; // Initialize all elements to 0

    if (isWiFiConnected())
    {
        additionalData[42] = 0x01; // Index 43 in array (arrays are zero-based)
    }

    sendAck(0x1D, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void getDashboardInfoBowl(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t additionalData[] = {
        // Device Name ASCII Chars ("DeviceName")
        0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x4E, 0x61,
        // Device Model Type Code
        0x05,
        // Application Firmware major version
        0x01,
        // Application Firmware minor version
        0x0,
        // Application Firmware patch version
        0x0,
        // Battery level
        0x2E,
        // Battery voltage higher byte
        0x01,
        // Battery voltage lower byte
        0x02,
        // Input Power higher byte
        0x02,
        // Input Power lower byte
        0x06,
        // Temperature higher byte
        0x07,
        // Temperature lower byte
        0x08};
    sendAck(0x16, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void setDogSize(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t additionalData[] = {0x00};
    sendAck(0x1A, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void ReadRecordingSessionDetails(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t uid[8];
    for (int i = 0; i < 8; ++i)
    {
        uid[i] = data[i + 4];
    }

    size_t additionalDataSize = 10;
    uint8_t *additionalData = (uint8_t *)malloc(additionalDataSize * sizeof(uint8_t));
    if (additionalData == NULL)
    {
        // Handle memory allocation failure
        return;
    }

    for (int i = 0; i < 8; ++i)
    {
        additionalData[i] = uid[i];
    }
    additionalData[8] = 0x00;
    additionalData[9] = 0x01;

    sendAck(0x11, additionalData, additionalDataSize, readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void ReadRecordingSessionData(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    // Extract UID from the data
    uint8_t uid[8];
    memcpy(uid, data + 4, 8);

    // Prepare myVector
    uint8_t *myVector = (uint8_t *)malloc(253 * sizeof(uint8_t));
    if (myVector == NULL)
    {
        // Handle memory allocation failure
        return;
    }
    memset(myVector, 0, 253 * sizeof(uint8_t)); // Initialize all elements to 0
    myVector[0] = 0x02;
    myVector[1] = 0x01;
    memcpy(myVector + 5, uid, 8);

    // Append checksum and update length
    appendChecksumAndUpdateLength(myVector, (size_t)253);

    // Set the second byte to 0x01
    myVector[1] = 0x01;

    // Call the readCallback function with myVector
    readCallback(myVector, 253);

    // Free allocated memory
    free(myVector);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void ReadRecordingSessionDetailsActivity(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t uid[8];
    for (int i = 0; i < 8; ++i)
    {
        uid[i] = data[i + 4];
    }

    size_t additionalDataSize = 10;
    uint8_t *additionalData = (uint8_t *)malloc(additionalDataSize * sizeof(uint8_t));
    if (additionalData == NULL)
    {
        // Handle memory allocation failure
        return;
    }

    for (int i = 0; i < 8; ++i)
    {
        additionalData[i] = uid[i];
    }
    additionalData[8] = 0x00;
    additionalData[9] = 0x01;

    sendAck(0x13, additionalData, additionalDataSize, readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void ReadRecordingSessionDataActivity(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    // Extract UID from the data
    uint8_t uid[8];
    memcpy(uid, data + 4, 8);

    // Prepare myVector
    uint8_t *myVector = (uint8_t *)malloc(253 * sizeof(uint8_t));
    if (myVector == NULL)
    {
        // Handle memory allocation failure
        return;
    }
    memset(myVector, 0, 253 * sizeof(uint8_t)); // Initialize all elements to 0
    myVector[0] = 0x02;
    myVector[1] = 0x01;
    memcpy(myVector + 5, uid, 8);

    // Append checksum and update length
    appendChecksumAndUpdateLength(myVector, (size_t)253);

    // Set the second byte to 0x01
    myVector[1] = 0x01;

    // Call the readCallback function with myVector
    readCallback(myVector, 253);

    // Free allocated memory
    free(myVector);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void getTotalSessionsCount(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t additionalData[1] = {0x05};
    sendAck(0x1E, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void getSessionByIndex(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    uint8_t element = data[4];
    int index = (int)element;

    if (index >= 0 && index < 5)
    {
        uint8_t additionalData[27];
        for (int i = 0; i < 27; ++i)
        {
            additionalData[i] = sessionData[index][i];
        }
        sendAck(0x1F, additionalData, sizeof(additionalData), readCallback);
    }
    else
    {
        printf("\n BLE_HOOK_INFO :Invalid session index\n");
    }
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void setSensorForBowl(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t))
{
    // Prepare additionalData
    uint8_t additionalData[] = {0x00};
    uint8_t selection = data[4];
    int count;
    const char **selectedSensors = decodeSelection(selection, &count);

    printf("\n BLE_HOOK_INFO :Selected sensors: ");
    for (int i = 0; i < count; i++)
    {
        printf("%s, ", selectedSensors[i]);
    }
    printf("\n");

    // Don't forget to free the allocated memory
    free(selectedSensors);

    // Call sendAck with appropriate parameters
    sendAck(0x20, additionalData, sizeof(additionalData), readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void appendChecksumAndUpdateLength(uint8_t *packet, size_t packet_size)
{
    packet[1] = (uint8_t)(packet_size + 2); // Update frame length
    uint16_t checksum = calculateChecksum(packet, packet_size);
    packet[packet_size] = (uint8_t)((checksum >> 8) & 0xFF); // CRC high byte
    packet[packet_size + 1] = (uint8_t)(checksum & 0xFF);    // CRC low byte
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
uint16_t calculateChecksum(const uint8_t *data, size_t data_length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < data_length; i++)
    {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void setTimeSync(const uint8_t *data, size_t dataSize, void (*readCallback)(const uint8_t *, size_t))
{
    const uint8_t additionalData[] = {0x00};
    size_t additionalDataSize = sizeof(additionalData) / sizeof(additionalData[0]);

    sendAck(0x06, additionalData, additionalDataSize, readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void getTimeSync(const uint8_t *data, size_t dataSize, void (*readCallback)(const uint8_t *, size_t))
{
    const uint8_t additionalData[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t additionalDataSize = sizeof(additionalData) / sizeof(additionalData[0]);

    sendAck(0x05, additionalData, additionalDataSize, readCallback);
}

/*******************************************************************************
 * @name
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/

void sendAck(uint8_t cmd, const uint8_t *data, size_t data_length, void (*readOrNotifyCallback)(const uint8_t *, size_t))
{
    uint8_t ackPacket[100];
    size_t ackPacketSize = 0;

    ackPacket[ackPacketSize++] = 0x02; // SOH
    ackPacket[ackPacketSize++] = 0x08; // Frame Length
    ackPacket[ackPacketSize++] = 0xF2; // FT
    ackPacket[ackPacketSize++] = cmd;  // CMD

    if (cmd == 0x04)
    {
        if (isWiFiConnected())
        {
            ackPacket[ackPacketSize++] = 0x11; // ACK
        }
        else
        {
            ackPacket[ackPacketSize++] = 0x12; // Error
        }
    }
    else
    {
        ackPacket[ackPacketSize++] = 0x11; // ACK
    }

    for (size_t i = 0; i < data_length; ++i)
    {
        ackPacket[ackPacketSize++] = data[i];
    }

    appendChecksumAndUpdateLength(ackPacket, ackPacketSize);

    readOrNotifyCallback(ackPacket, ackPacketSize + 2);
}