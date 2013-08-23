#ifndef QBRT_INSTRUCTION_TYPE_H
#define QBRT_INSTRUCTION_TYPE_H

#include "qbrt/core.h"


struct ctuple_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t dst : 16;
	uint32_t size : 8;

	ctuple_instruction(reg_t dst, uint8_t siz)
		: opcode_data(OP_CTUPLE)
		, dst(dst)
		, size(siz)
	{}

	static const uint8_t SIZE = 4;
};

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


/** Construct a list. Don't need this w/ the list.Empty construct */
struct clist_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t dst : 16;

	clist_instruction(reg_t dst)
		: opcode_data(OP_CLIST)
		, dst(dst)
	{}

	static const uint8_t SIZE = 3;
};

/** Push an item on to the front of the list */
struct cons_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t head : 16;
	uint32_t item : 16;

	cons_instruction(reg_t head, reg_t item)
		: opcode_data(OP_CLIST)
		, head(head)
		, item(item)
	{}

	static const uint8_t SIZE = 5;
};

#endif
