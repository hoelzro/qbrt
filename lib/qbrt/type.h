#ifndef QBRT_TYPE_H
#define QBRT_TYPE_H

#include "qbrt/core.h"
#include <cstdio>
#include <list>


struct StructFieldResource
{
	uint16_t type_mod_id;
	uint16_t type_name_id;
	uint16_t field_name_id;

	static const uint32_t SIZE = 6;
};

struct StructResource
{
	uint16_t name_id;
	uint16_t field_count;
	StructFieldResource field[];
};

struct StructField
{
	Type* type;
	std::string name;
};

struct Type
{
	std::string module;
	std::string name;
	uint8_t id;
	std::vector< StructField > field;

	Type(uint8_t id);
	Type(const std::string &mod, const std::string &name, uint16_t flds);
};


struct Promise
{
	std::list< TaskID > waiter;
	TaskID promiser;

	Promise(TaskID tid)
	: promiser(tid)
	{}
};

#endif
