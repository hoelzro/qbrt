#ifndef QBRT_INSTRUCTION_ARITHMETIC_H
#define QBRT_INSTRUCTION_ARITHMETIC_H

#include "qbrt/core.h"

#pragma pack(push, 1)

struct binaryop_instruction
: public instruction
{
	uint16_t result;
	uint16_t a;
	uint16_t b;

	binaryop_instruction(uint8_t op, reg_t result, reg_t a, reg_t b)
	: instruction(op)
	, result(result)
	, a(a)
	, b(b)
	{}

	static const uint8_t SIZE = 7;
};

struct consti_instruction
: public instruction
{
	uint16_t reg;
	int32_t value;

	consti_instruction(reg_t reg, int32_t value)
		: instruction(OP_CONSTI)
		, reg(reg)
		, value(value)
	{}

	static const uint8_t SIZE = 7;
};

#pragma pack(pop)

#endif
