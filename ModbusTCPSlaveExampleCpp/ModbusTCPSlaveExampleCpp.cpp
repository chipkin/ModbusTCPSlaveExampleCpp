// ModbusTCPSlaveExampleCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// CASModbusSlaveTCPExample
//
// Last updated: 2018 Aug 1
//

// #include <conio.h>
#include <stdio.h>
#include <time.h> // Current time in seconds.

#include "CTCP.h"                   // TCP used by Modbus TCP
#include "CExampleModbusDatabase.h" // Example Modbus database of values. 
#include "CASModbusAdapter.h"		// Loads all the Modbus functions. 
#include "ChipkinEndianness.h"		// Helps with OS depenent Endianness

using namespace ChipkinCommon;

// Settings
// ===========================
const unsigned short SETTING_TCP_PORT = 502;
const unsigned char SETTING_MODBUS_SERVER_SLAVE_ADDRESS = 0x00;
// MODBUS_TYPE_RTU = 1 || MODBUS_TYPE_TCP = 2
unsigned int SETTING_MODBUS_TYPE = 2;

// Globals
// ===========================
// TCP connection
CTCP gTcp;
CModbusDatabase gDatabase;


// API return codes.
// ===========================
static const unsigned int MODBUS_STATUS_ERROR_UNKNOWN = 0;
static const unsigned int MODBUS_STATUS_SUCCESS = 1;

// Modbus function codes
// ===========================
static const unsigned char MODBUS_FUNCTION_01_READ_COIL_STATUS = 1;
static const unsigned char MODBUS_FUNCTION_02_READ_INPUT_STATUS = 2;
static const unsigned char MODBUS_FUNCTION_03_READ_HOLDING_REGISTERS = 3;
static const unsigned char MODBUS_FUNCTION_04_READ_INPUT_REGISTERS = 4;
static const unsigned char MODBUS_FUNCTION_05_FORCE_SINGLE_COIL = 5;
static const unsigned char MODBUS_FUNCTION_06_PRESET_SINGLE_REGISTER = 6;
static const unsigned char MODBUS_FUNCTION_0F_FORCE_MULTIPLE_COILS = 15;
static const unsigned char MODBUS_FUNCTION_10_FORCE_MULTIPLE_REGISTERS = 16;
static const unsigned short MODBUS_FORCE_SINGLE_COIL_ON = 0xFF00;
static const unsigned short MODBUS_FORCE_SINGLE_COIL_OFF = 0x0000;

// Modbus exception codes
// ===========================
static const unsigned int MODBUS_STATUS_EXCEPTION_01_ILLEGAL_FUNCTION = 0x01;
static const unsigned int MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS = 0x02;
static const unsigned int MODBUS_STATUS_EXCEPTION_04_SLAVE_DEVICE_FAILURE = 0x04;

// Helper functions
// ===========================

unsigned char SetBit(unsigned char data, unsigned char bit, bool value)
{
	// http://stackoverflow.com/a/47990
	data ^= (-(value ? 1 : 0) ^ data) & (1 << bit);
	return data;
}
bool GetBit(unsigned char data, unsigned char bit)
{
	return ((data >> bit) & 1);
}

#ifdef LINUX
#include <unistd.h>
#endif
#ifdef WINDOWS
#include <windows.h>
#endif

void mySleep(int sleepMs)
{
#ifdef LINUX
	usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
#ifdef WINDOWS
	Sleep(sleepMs);
#endif
}



// Callback functions
// ===========================
bool sendModbusMessage(const unsigned short connectionId, const unsigned char* payload, const unsigned short payloadSize)
{
	// Debug print sent message.
	printf("FYI: TX: ");
	for (unsigned int offset = 0; offset < payloadSize; offset++)
	{
		printf("%02X ", payload[offset]);
	}
	printf("\n");

	if (!gTcp.SendMessage(connectionId, payload, payloadSize))
	{
		printf("Error: SendMessage failed.\n");
		return false; // Unknonw error
	}
	// Everything looks good.
	return true;
}
unsigned int recvModbusMessage(unsigned short& connectionId, unsigned char* payload, unsigned short maxPayloadSize)
{
	if (maxPayloadSize <= 0 || payload == NULL)
	{
		return 0; // No space to store message.
	}
	int length = gTcp.GetMessage(connectionId, payload, maxPayloadSize);
	if (length <= 0)
	{
		return 0;
	}

	// Debug print recived message.
	printf("FYI: RX: ");
	for (unsigned int offset = 0; offset < length; offset++)
	{
		printf("%02X ", payload[offset]);
	}
	printf("\n");

	// Everything looks good.
	return length;
}
unsigned long currentTime()
{
	return (unsigned long)time(0);
}

bool setModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, const unsigned char* data, const unsigned short dataSize, unsigned char* errorCode)
{
	// Check to see if this request is for us?
	if (SETTING_MODBUS_SERVER_SLAVE_ADDRESS != slaveAddress)
	{
		return false;
	}

	printf("FYI: Set Modbus Value. slaveAddress=[%d], function=[%d], startingAddress=[%d], length=[%d], dataSize=[%d]\n", slaveAddress, function, startingAddress, length, dataSize);
	switch (function)
	{
	case MODBUS_FUNCTION_05_FORCE_SINGLE_COIL:
	{
		if (startingAddress > gDatabase.GetCoilCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}

		unsigned short value = 0x0000;
		memcpy(&value, data, sizeof(short));

		unsigned char byteOffset = startingAddress / 8;
		unsigned char bitOffset = startingAddress % 8;

		if (value == MODBUS_FORCE_SINGLE_COIL_ON)
		{
			gDatabase.m_coils[byteOffset] |= (1 << bitOffset);
		}
		else
		{
			gDatabase.m_coils[byteOffset] &= ~(1 << bitOffset);
		}

		// Everyting is looking good.
		return true;
		break;
	}
	case MODBUS_FUNCTION_06_PRESET_SINGLE_REGISTER:
	{
		if (startingAddress > gDatabase.GetHoldingRegistersCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}

		// Extract the value.
		unsigned short value = 0x0000;
		memcpy(&value, data, sizeof(short));

		// Set the value
		gDatabase.m_holdingRegisters[startingAddress] = value;

		// Everyting is looking good.
		return true;
		break;
	}
	case MODBUS_FUNCTION_0F_FORCE_MULTIPLE_COILS:
	{
		if (startingAddress + length > gDatabase.GetCoilCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}

		// Extract and set the value into the database.
		for (int offset = 0; offset < length; offset++)
		{
			unsigned char currentByteOffset = (startingAddress + offset) / 8;
			unsigned char currentBitOffset = (startingAddress + offset) % 8;

			unsigned char dataByteOffset = offset / 8;
			unsigned char dataBitOffset = offset % 8;

			// Store the value in the database.
			gDatabase.m_coils[currentByteOffset] = SetBit(gDatabase.m_coils[currentByteOffset], currentBitOffset, GetBit(data[dataByteOffset], dataBitOffset));
		}

		// Everyting is looking good.
		return true;
		break;
	}
	case MODBUS_FUNCTION_10_FORCE_MULTIPLE_REGISTERS:
	{

		if (startingAddress + length > gDatabase.GetHoldingRegistersCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}

		// Extract the values and copy it to the database.
		unsigned short value = 0x00;
		for (int offset = 0; offset < length; offset++)
		{
			memcpy(&value, data + offset * sizeof(short), sizeof(short));
			gDatabase.m_holdingRegisters[startingAddress + offset] = value;
		}

		// Everyting is looking good.
		return true;
		break;
	}
	default:
	{
		*errorCode = MODBUS_STATUS_EXCEPTION_01_ILLEGAL_FUNCTION;
		return false;
		break;
	}
	}

	// Unknown error
	*errorCode = MODBUS_STATUS_EXCEPTION_04_SLAVE_DEVICE_FAILURE;
	return false;
}
bool getModbusValue(const unsigned char slaveAddress, const unsigned char function, const unsigned short startingAddress, const unsigned short length, unsigned char* data, const unsigned short maxPayloadSize, unsigned char* errorCode)
{
	// Check to see if this request is for us?
	if (SETTING_MODBUS_SERVER_SLAVE_ADDRESS != slaveAddress)
	{
		return false;
	}
	printf("FYI: Get Modbus Value. slaveAddress=[%d], function=[%d], startingAddress=[%d], length=[%d], dataSize=[%d]\n", slaveAddress, function, startingAddress, length, maxPayloadSize);

	switch (function)
	{
	case MODBUS_FUNCTION_01_READ_COIL_STATUS:
	{
		if (startingAddress > gDatabase.GetCoilCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}

		memcpy(data, gDatabase.m_coils + (startingAddress / 8), length);
		return true;
		break;
	}
	case MODBUS_FUNCTION_02_READ_INPUT_STATUS:
	{
		if (startingAddress > gDatabase.GetInputCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}
		memcpy(data, gDatabase.m_input + (startingAddress / 8), length);
		return true;
		break;
	}
	case MODBUS_FUNCTION_03_READ_HOLDING_REGISTERS:
	{
		if (startingAddress > gDatabase.GetHoldingRegistersCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}
		memcpy(data, gDatabase.m_holdingRegisters + startingAddress, length * sizeof(short));
		return true;
		break;
	}
	case MODBUS_FUNCTION_04_READ_INPUT_REGISTERS:
	{
		if (startingAddress > gDatabase.GetInputRegistersCount())
		{
			// Out of range
			*errorCode = MODBUS_STATUS_EXCEPTION_02_ILLEGAL_DATA_ADDRESS;
			return false;
		}
		memcpy(data, gDatabase.m_inputRegisters + startingAddress, length * sizeof(short));
		return true;
		break;
	}
	default:
	{
		*errorCode = MODBUS_STATUS_EXCEPTION_01_ILLEGAL_FUNCTION;
		return false;
		break;
	}
	}

	// Unknown error
	*errorCode = MODBUS_STATUS_EXCEPTION_04_SLAVE_DEVICE_FAILURE;
	return false;
}

int main()
{

	printf("FYI: Modbus TCP Example CPP. version: 2024-Aug-02\n");
	printf("https://github.com/chipkin/ModbusTCPSlaveExampleCpp\n");

	// Load the DLL functions
	// --------------------------------
	if (!LoadModbusFunctions())
	{
		printf("Error: Could not load DLL functions\n");
		return 1;
	}
	printf("FYI: Modbus Stack version: %d.%d.%d.%d\n", fpModbusStack_GetAPIMajorVersion(), fpModbusStack_GetAPIMinorVersion(), fpModbusStack_GetAPIPatchVersion(), fpModbusStack_GetAPIBuildVersion());

	// Set up the API and callbacks.
	// --------------------------------
	unsigned int returnCode = fpModbusStack_Init(SETTING_MODBUS_TYPE, sendModbusMessage, recvModbusMessage, currentTime);
	if (returnCode != MODBUS_STATUS_SUCCESS)
	{
		printf("Error: Could not init the Modbus Stack, returnCode=%d\n", returnCode);
		return 1;
	}
	printf("FYI: Modbus stack init, successfuly\n");

	fpModbusStack_SetSlaveId(SETTING_MODBUS_SERVER_SLAVE_ADDRESS);

	// Set up the Modbus server call backs
	fpModbusStack_RegisterGetValue(getModbusValue);
	fpModbusStack_RegisterSetValue(setModbusValue);

	// Generate an example database
	gDatabase.SampleData();

	printf("FYI: Slave Address: %d\n", SETTING_MODBUS_SERVER_SLAVE_ADDRESS);
	printf("FYI: GetCoilCount: %d\n", gDatabase.GetCoilCount());
	printf("FYI: InputCount: %d\n", gDatabase.GetInputCount());
	printf("FYI: GetHoldingRegistersCount: %d\n", gDatabase.GetHoldingRegistersCount());
	printf("FYI: GetInputRegistersCount: %d\n", gDatabase.GetInputRegistersCount());
	

	// Set up the socket to listen for new incoming connections.
	if (!gTcp.Listen(SETTING_TCP_PORT))
	{
		printf("\n");
		printf("Error: Can not listen to TCP port=[%d]... Is there a Modbus server already running?\n", SETTING_TCP_PORT);
		return 1;
	}
	printf("FYI: ConnectionMax: %d\n", gTcp.GetMaxConnections());
	printf("FYI: Waiting on TCP connection TCP port=[%d]... \n", SETTING_TCP_PORT);

	char ipAddress[100];
	unsigned short port = 0;

	for (;;)
	{
		// Debug: Check to see if any connections have been disconnected. 
		static size_t lastNumberOfConnections = 0;
		if (lastNumberOfConnections != gTcp.GetNumberOfConnections()) {
			lastNumberOfConnections = gTcp.GetNumberOfConnections();
			printf("FYI: The number of connections changed to [%d]\n", lastNumberOfConnections);
		}

		// Check to see if we have any incoming messages.
		if (gTcp.Accept(ipAddress, &port))
		{
			// We have recived a NEW connection.
			printf("FYI: Got a connection from IP address=[%s] port=[%d]... \n", ipAddress, port);
		}

		// Run the Modbus loop proccessing incoming messages.
		fpModbusStack_Loop();

		// Give some time back to the CPU
		mySleep(0);

		// Finally flush the buffer
		fpModbusStack_Flush();
	}
	return 0;
}
