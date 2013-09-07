#ifndef QBRT_ARITHMETIC_H
#define QBRT_ARITHMETIC_H

#include "core.h"


struct binaryop_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t result : 16;
	uint64_t a : 16;
	uint64_t b : 16;

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
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	int64_t value : 32;

	consti_instruction(reg_t reg, uint32_t value)
		: opcode_data(OP_CONSTI)
		, reg(reg)
		, value(value)
	{}

	static const uint8_t SIZE = 7;
};

#endif
