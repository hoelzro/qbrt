#ifndef QBRT_TUPLE_H
#define QBRT_TUPLE_H

#include <string>
#include "core.h"


struct Tuple
: public qbrt_value_index
{
	qbrt_value *data;
	uint16_t size;

	Tuple(uint16_t sz)
		: data(new qbrt_value[sz])
		, size(sz)
	{}

	~Tuple()
	{
		delete[] data;
	}

	virtual uint8_t num_values() const { return size; }
	virtual qbrt_value & value(uint16_t i) { return data[i]; }
	virtual const qbrt_value & value(uint16_t i) const { return data[i]; }
};

struct ctuple_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t target : 16;
	uint32_t size : 8;

	ctuple_instruction(reg_t dst, uint8_t siz)
		: opcode_data(OP_CTUPLE)
		, target(dst)
		, size(siz)
	{}

	static const uint8_t SIZE = 4;
};

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

/*
struct UserType {
	std::string name;
	vector< TypeField > field;
};

struct TypedStruct {
	UserType *type;
	qbrt_value *values;
};
*/

#endif
