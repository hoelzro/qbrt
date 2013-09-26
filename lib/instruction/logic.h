#ifndef QBRT_INSTRUCTION_LOGIC_H
#define QBRT_INSTRUCTION_LOGIC_H

#include "qbrt/core.h"

#pragma pack(push, 1)


struct cmp_instruction
: public instruction
{
	uint16_t result;
	uint16_t a;
	uint16_t b;

	cmp_instruction(int8_t opcode, reg_t result, reg_t a, reg_t b)
	: instruction(opcode)
	, result(result)
	, a(a)
	, b(b)
	{}

	static const uint8_t SIZE = 7;
};


#pragma pack(pop)

#endif
