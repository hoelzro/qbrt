#ifndef QBRT_ARITHMETIC_H
#define QBRT_ARITHMETIC_H

#include "core.h"

#pragma pack(push, 1)

struct binaryop_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t result;
	uint16_t a;
	uint16_t b;

	binaryop_instruction(uint8_t op, reg_t result, reg_t a, reg_t b)
	: opcode_data(op)
	, result(result)
	, a(a)
	, b(b)
	{}

	static const uint8_t SIZE = 7;
};

struct consti_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t reg;
	int32_t value;

	consti_instruction(reg_t reg, uint32_t value)
		: opcode_data(OP_CONSTI)
		, reg(reg)
		, value(value)
	{}

	static const uint8_t SIZE = 7;
};

#pragma pack(pop)

#endif
