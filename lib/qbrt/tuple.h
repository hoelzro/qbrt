#ifndef QBRT_TUPLE_H
#define QBRT_TUPLE_H

#include <string>
#include "core.h"


#pragma pack(push, 1)

struct stuple_instruction
: public instruction
{
	uint16_t tuple;
	uint16_t index;
	uint16_t data;

	stuple_instruction(reg_t tup, uint16_t idx, uint16_t dat)
		: instruction(OP_STUPLE)
		, tuple(tup)
		, index(idx)
		, data(dat)
	{}

	static const uint8_t SIZE = 7;
};

#pragma pack(pop)

#endif
