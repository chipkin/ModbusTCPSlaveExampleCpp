#pragma once

#ifndef __CExampleModbusDatabase__
#define __CExampleModbusDatabase__

#include <map>
class CModbusDatabase {
private:
	// Constantans
	static const unsigned short MAX_COILS = 2000;
	static const unsigned short MAX_INPUTS = 2000;
	static const unsigned short MAX_HOLDING_REGISTERS = 10000;
	static const unsigned short MAX_INPUT_REGISTERS = 10000;

public:

	unsigned char m_coils[MAX_COILS / 8];
	unsigned char m_input[MAX_INPUTS / 8];
	unsigned short m_holdingRegisters[MAX_HOLDING_REGISTERS];
	unsigned short m_inputRegisters[MAX_INPUT_REGISTERS];

	// Accessors 
	unsigned short GetCoilCount() const { return MAX_COILS; }
	unsigned short GetInputCount() const { return MAX_INPUTS; }
	unsigned short GetHoldingRegistersCount() const { return MAX_HOLDING_REGISTERS; }
	unsigned short GetInputRegistersCount() const { return MAX_INPUT_REGISTERS; }

	CModbusDatabase()
	{
		this->Setup();
	}
	void Setup()
	{
		// Clear the database.
		memset(this->m_coils, 0, MAX_COILS / 8);
		memset(this->m_input, 0, MAX_INPUTS / 8);
		memset(this->m_holdingRegisters, 0, MAX_HOLDING_REGISTERS * sizeof(short));
		memset(this->m_inputRegisters, 0, MAX_INPUT_REGISTERS * sizeof(short));
	}

	void SampleData()
	{
		for (unsigned short offset = 0; offset < this->GetHoldingRegistersCount(); offset++) {
			this->m_holdingRegisters[offset] = offset;
		}
		for (unsigned short offset = 0; offset < this->GetInputRegistersCount(); offset++) {
			this->m_inputRegisters[offset] = offset;
		}
	}
};

#endif // __CExampleModbusDatabase__