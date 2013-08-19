#ifndef QBRT_TUPLE_H
#define QBRT_TUPLE_H

#include <string>
#include "core.h"


struct stuple_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t tuple : 16;
	uint64_t index : 16;
	uint64_t data  : 16;

	stuple_instruction(reg_t tup, uint16_t idx, uint16_t dat)
		: opcode_data(OP_STUPLE)
		, tuple(tup)
		, index(idx)
		, data(dat)
	{}

	static const uint8_t SIZE = 7;
};

#endif
