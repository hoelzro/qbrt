#ifndef LOGIC_H
#define LOGIC_H

#include "core.h"

#pragma pack(push, 1)

struct goto_instruction
: public jump_instruction
{
	goto_instruction()
	: jump_instruction(OP_GOTO)
	{}

	static const uint8_t SIZE = 3;
};

struct if_instruction
: public jump_instruction
{
	uint16_t op;

	if_instruction(bool check, reg_t op)
	: jump_instruction(check ? OP_IF : OP_IFNOT)
	, op(op)
	{}

	inline bool ifnot() const { return opcode_data == OP_IFNOT; }

	static const uint8_t SIZE = 5;
};

struct ifcmp_instruction
: public jump_instruction
{
	uint16_t ra;
	uint16_t rb;

	ifcmp_instruction(int8_t opcode, reg_t a, reg_t b)
	: jump_instruction(opcode)
	, ra(a)
	, rb(b)
	{}

	static const uint8_t SIZE = 7;
};

struct iffail_instruction
: public jump_instruction
{
	uint16_t op;

	iffail_instruction(bool check, reg_t op)
	: jump_instruction(check ? OP_IFFAIL : OP_IFNOTFAIL)
	, op(op)
	{}

	inline bool iffail() const { return opcode_data == OP_IFFAIL; }
	inline bool ifnotfail() const { return !iffail(); }

	static const uint8_t SIZE = 5;
};

struct match_instruction
: public jump_instruction
{
	uint16_t result;
	uint16_t pattern;
	uint16_t input;

	match_instruction(reg_t result, reg_t patt, reg_t in)
	: jump_instruction(OP_MATCH)
	, result(result)
	, pattern(patt)
	, input(in)
	{}

	static const uint8_t SIZE = 9;
};

struct matchargs_instruction
: public jump_instruction
{
	uint16_t result;
	uint16_t pattern;

	matchargs_instruction(reg_t result, reg_t patt)
	: jump_instruction(OP_MATCHARGS)
	, result(result)
	, pattern(patt)
	{}

	static const uint8_t SIZE = 7;
};

struct fork_instruction
: public jump_instruction
{
	uint16_t result;

	fork_instruction(reg_t result)
	: jump_instruction(OP_FORK)
	, result(result)
	{}

	static const uint8_t SIZE = 5;
};

struct wait_instruction
: public instruction
{
	uint16_t reg;

	wait_instruction(reg_t r)
	: instruction(OP_WAIT)
	, reg(r)
	{}

	static const uint8_t SIZE = 3;
};

#pragma pack(pop)

#endif
