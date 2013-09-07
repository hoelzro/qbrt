#ifndef QBRT_STRING_H
#define QBRT_STRING_H

#include <string>
#include "qbrt/core.h"


struct consts_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t string_id : 16;

	consts_instruction(reg_t reg, uint16_t str_id)
		: opcode_data(OP_CONSTS)
		, reg(reg)
		, string_id(str_id)
	{}

	static const uint8_t SIZE = 5;
};

struct consthash_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t hash_id : 16;

	consthash_instruction(reg_t reg, uint16_t hash_id)
		: opcode_data(OP_CONSTHASH)
		, reg(reg)
		, hash_id(hash_id)
	{}

	static const uint8_t SIZE = 5;
};

struct stracc_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t dst : 16;
	uint64_t src : 16;

	stracc_instruction(reg_t dst, reg_t src)
		: opcode_data(OP_STRACC)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};


struct StringResource
{
	uint16_t length;
	const char value[];
};

struct HashTagResource
{
	uint16_t length;
	const char value[];
};

#endif
