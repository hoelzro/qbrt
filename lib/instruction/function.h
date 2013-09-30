#ifndef QBRT_INSTRUCTION_FUNCTION_H
#define QBRT_INSTRUCTION_FUNCTION_H

#include "qbrt/core.h"


#pragma pack(push, 1)

struct call_instruction
: public instruction
{
	uint16_t result_reg;
	uint16_t func_reg;

	call_instruction(reg_t result, reg_t func)
		: instruction(OP_CALL)
		, result_reg(result)
		, func_reg(func)
	{}

	static const uint8_t SIZE = 5;
};

struct return_instruction
: public instruction
{
	return_instruction()
	: instruction(OP_RETURN)
	{}

	static const uint8_t SIZE = 1;
};

struct cfailure_instruction
: public instruction
{
	uint16_t dst;
	uint16_t hashtag_id;

	cfailure_instruction(reg_t dst, uint16_t hash_id)
	: instruction(OP_CFAILURE)
	, dst(dst)
	, hashtag_id(hash_id)
	{}

	static const uint8_t SIZE = 5;
};

struct lcontext_instruction
: public instruction
{
	uint16_t reg;
	uint16_t hashtag;

	lcontext_instruction(reg_t r, uint16_t hashtag)
		: instruction(OP_LCONTEXT)
		, reg(r)
		, hashtag(hashtag)
	{}

	static const uint8_t SIZE = 5;
};

struct lfunc_instruction
: public instruction
{
	uint16_t reg;
	uint16_t modsym;

	lfunc_instruction(reg_t r, uint16_t modsym)
		: instruction(OP_LFUNC)
		, reg(r)
		, modsym(modsym)
	{}

	static const uint8_t SIZE = 5;
};

struct loadtype_instruction
: public instruction
{
	uint16_t reg;
	uint16_t modname;
	uint16_t type;

	loadtype_instruction(reg_t r, uint16_t mod, uint16_t type)
		: instruction(OP_LFUNC)
		, reg(r)
		, modname(mod)
		, type(type)
	{}

	static const uint8_t SIZE = 7;
};

struct loadobj_instruction
: public instruction
{
	uint16_t modname;

	loadobj_instruction(uint16_t mod)
		: instruction(OP_LOADOBJ)
		, modname(mod)
	{}

	static const uint8_t SIZE = 3;
};

struct move_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	move_instruction(reg_t dst, reg_t src)
		: instruction(OP_MOVE)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

struct ref_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	ref_instruction(reg_t dst, reg_t src)
		: instruction(OP_REF)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

struct copy_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	copy_instruction(reg_t dst, reg_t src)
		: instruction(OP_COPY)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

#pragma pack(pop)



#endif
