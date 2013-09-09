#ifndef QBRT_STRING_H
#define QBRT_STRING_H

#include <string>
#include "qbrt/core.h"


#pragma pack(push, 1)

struct consts_instruction
: public instruction
{
	uint16_t reg;
	uint16_t string_id;

	consts_instruction(reg_t reg, uint16_t str_id)
		: instruction(OP_CONSTS)
		, reg(reg)
		, string_id(str_id)
	{}

	static const uint8_t SIZE = 5;
};

struct consthash_instruction
: public instruction
{
	uint16_t reg;
	uint16_t hash_id;

	consthash_instruction(reg_t reg, uint16_t hash_id)
		: instruction(OP_CONSTHASH)
		, reg(reg)
		, hash_id(hash_id)
	{}

	static const uint8_t SIZE = 5;
};

struct stracc_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	stracc_instruction(reg_t dst, reg_t src)
		: instruction(OP_STRACC)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

#pragma pack(pop)


struct StringResource
{
	uint16_t bytes;
	const char value[];
};

struct HashTagResource
{
	uint16_t length;
	const char value[];
};

#endif
