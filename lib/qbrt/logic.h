#ifndef LOGIC_H
#define LOGIC_H

#include "core.h"


struct goto_instruction
: public jump_instruction
{
	int32_t opcode_data : 8;
	int32_t jump_data : 16;
	int32_t reserved : 8;

	goto_instruction(int16_t jump)
		: opcode_data(OP_GOTO)
		, jump_data(jump)
	{}

	static const uint8_t SIZE = 3;
};

struct brbool_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t op : 16;
	int64_t reserved : 24;

	brbool_instruction(bool check, int16_t jmp, reg_t op)
		: opcode_data(check ? OP_BRT : OP_BRF)
		, jump_data(jmp)
		, op(op)
	{}

	inline bool brt() const { return opcode_data == OP_BRT; }
	inline bool brf() const { return opcode_data == OP_BRF; }

	static const uint8_t SIZE = 5;
};

struct brcmp_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t ra : 16;
	int64_t rb : 16;
	int64_t reserved : 8;

	brcmp_instruction(int8_t opcode, reg_t a, reg_t b)
		: opcode_data(opcode)
		, jump_data(0)
		, ra(a)
		, rb(b)
	{}

	static const uint8_t SIZE = 7;
};

struct breq_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t ra : 16;
	int64_t rb : 16;
	int64_t reserved : 8;

	breq_instruction(int16_t jmp, reg_t a, reg_t b)
		: opcode_data(OP_BREQ)
		, jump_data(jmp)
		, ra(a)
		, rb(b)
	{}

	static const uint8_t SIZE = 7;
};

struct brfail_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t op : 16;
	int64_t reserved : 24;

	brfail_instruction(bool check, reg_t op)
	: opcode_data(check ? OP_BRFAIL : OP_BRNFAIL)
	, jump_data(0)
	, op(op)
	, reserved(0)
	{}

	inline bool brfail() const { return opcode_data == OP_BRFAIL; }
	inline bool brnfail() const { return opcode_data == OP_BRNFAIL; }

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

struct fork_instruction
: public jump_instruction
{
	int64_t opcode_data : 8;
	int64_t jump_data : 16;
	int64_t result : 16;
	int64_t reserved : 24;

	fork_instruction(int16_t jmp, reg_t result)
	: opcode_data(OP_FORK)
	, jump_data(jmp)
	, result(result)
	, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct wait_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t reg : 16;
	uint32_t reserved : 8;

	wait_instruction(reg_t r)
	: opcode_data(OP_WAIT)
	, reg(r)
	, reserved(0)
	{}

	static const uint8_t SIZE = 3;
};

#endif
