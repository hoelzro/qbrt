#ifndef LOGIC_H
#define LOGIC_H

#include "core.h"


struct goto_instruction
: public jump_instruction
{
	int32_t opcode_data : 8;
	int32_t jump_data : 16;

	goto_instruction(int16_t jump)
		: opcode_data(OP_GOTO)
		, jump_data(jump)
	{}

	static const uint8_t SIZE = 3;
};

struct if_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t op : 16;

	if_instruction(bool check, reg_t op)
		: opcode_data(check ? OP_IF : OP_IFNOT)
		, jump_data(0)
		, op(op)
	{}

	inline bool ifnot() const { return opcode_data == OP_IFNOT; }

	static const uint8_t SIZE = 5;
};

struct ifcmp_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t ra : 16;
	int64_t rb : 16;

	ifcmp_instruction(int8_t opcode, reg_t a, reg_t b)
		: opcode_data(opcode)
		, jump_data(0)
		, ra(a)
		, rb(b)
	{}

	static const uint8_t SIZE = 7;
};

struct iffail_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t op : 16;

	iffail_instruction(bool check, reg_t op)
	: opcode_data(check ? OP_IFFAIL : OP_IFNOTFAIL)
	, jump_data(0)
	, op(op)
	{}

	inline bool iffail() const { return opcode_data == OP_IFFAIL; }
	inline bool ifnotfail() const { return opcode_data == OP_IFNOTFAIL; }

	static const uint8_t SIZE = 5;
};

struct match_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data :16;
	int64_t result : 16;
	int64_t pattern : 16;
	int64_t input : 16;

	match_instruction(reg_t result, reg_t patt, reg_t in)
	: opcode_data(OP_MATCH)
	, jump_data(0)
	, result(result)
	, pattern(patt)
	, input(in)
	{}

	static const uint8_t SIZE = 9;
};

struct matchargs_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data :16;
	int64_t result : 16;
	int64_t pattern : 16;

	matchargs_instruction(reg_t result, reg_t patt)
	: opcode_data(OP_MATCHARGS)
	, jump_data(0)
	, result(result)
	, pattern(patt)
	{}

	static const uint8_t SIZE = 7;
};

struct fork_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t result : 16;

	fork_instruction(int16_t jmp, reg_t result)
	: opcode_data(OP_FORK)
	, jump_data(jmp)
	, result(result)
	{}

	static const uint8_t SIZE = 5;
};

struct wait_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t reg : 16;

	wait_instruction(reg_t r)
	: opcode_data(OP_WAIT)
	, reg(r)
	{}

	static const uint8_t SIZE = 3;
};

#endif
