#ifndef QBRT_INSTRUCTION_TYPE_H
#define QBRT_INSTRUCTION_TYPE_H

#include "qbrt/core.h"


struct lconstruct_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t modsym : 16;
	uint64_t reserved : 24;

	lconstruct_instruction(reg_t r, uint16_t modsym)
		: opcode_data(OP_LCONSTRUCT)
		, reg(r)
		, modsym(modsym)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};


struct patternvar_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t dst : 16;
	uint32_t reserved : 8;

	patternvar_instruction(reg_t r)
	: opcode_data(OP_PATTERNVAR)
	, dst(r)
	, reserved(0)
	{}

	static const uint8_t SIZE = 3;
};


#endif
