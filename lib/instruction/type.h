#ifndef QBRT_INSTRUCTION_TYPE_H
#define QBRT_INSTRUCTION_TYPE_H

#include "qbrt/core.h"

#pragma pack(push, 1)

struct ctuple_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t dst;
	uint8_t size;

	ctuple_instruction(reg_t dst, uint8_t siz)
		: opcode_data(OP_CTUPLE)
		, dst(dst)
		, size(siz)
	{}

	static const uint8_t SIZE = 4;
};

struct fieldget_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t dst;
	uint16_t src;
	uint16_t field_name;

	fieldget_instruction(reg_t dst, reg_t src, uint16_t field_name)
	: opcode_data(OP_FIELDGET)
	, dst(dst)
	, src(src)
	, field_name(field_name)
	{}

	static const uint8_t SIZE = 7;
};

struct fieldset_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t dst;
	uint16_t field_name;
	uint16_t src;

	fieldset_instruction(reg_t dst, uint16_t field_name, reg_t src)
	: opcode_data(OP_FIELDSET)
	, dst(dst)
	, field_name(field_name)
	, src(src)
	{}

	static const uint8_t SIZE = 7;
};

struct lconstruct_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t reg;
	uint16_t modsym;

	lconstruct_instruction(reg_t r, uint16_t modsym)
		: opcode_data(OP_LCONSTRUCT)
		, reg(r)
		, modsym(modsym)
	{}

	static const uint8_t SIZE = 5;
};


struct patternvar_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t dst;

	patternvar_instruction(reg_t r)
	: opcode_data(OP_PATTERNVAR)
	, dst(r)
	{}

	static const uint8_t SIZE = 3;
};


/** Construct a list. Don't need this w/ the list.Empty construct */
struct clist_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t dst;

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
	uint8_t opcode_data;
	uint16_t head;
	uint16_t item;

	cons_instruction(reg_t head, reg_t item)
		: opcode_data(OP_CLIST)
		, head(head)
		, item(item)
	{}

	static const uint8_t SIZE = 5;
};

#pragma pack(pop)

#endif
