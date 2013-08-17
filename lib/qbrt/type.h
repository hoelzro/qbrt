#ifndef QBRT_TYPE_H
#define QBRT_TYPE_H

#include "qbrt/core.h"
#include <cstdio>
#include <list>


struct ParamResource
{
	uint16_t name_idx;
	uint16_t type_idx;
};

struct TypeSpecResource
{
	uint16_t name_idx;	// modsym
	uint16_t fullname_idx;	// string
	uint16_t args[];	// other TypeSpecResource indexes
};

struct ConstructResource
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t filename_idx;
	uint16_t lineno;
	uint16_t datatype_idx;
	uint8_t fld_count;
	uint8_t reserved;
	ParamResource fields[];

	static const uint32_t SIZE = 12;
};

struct DataTypeResource
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t filename_idx;
	uint16_t lineno;
	uint8_t argc;

	static const uint32_t SIZE = 9;
};

struct Construct
: public qbrt_value_index
{
	const ConstructResource &resource;
	qbrt_value *fields;

	Construct(const ConstructResource &cr)
	: resource(cr)
	, fields(new qbrt_value[cr.fld_count])
	{}
	~Construct()
	{
		delete[] fields;
	}

	friend bool operator < (const Construct &a, const Construct &b)
	{
		return a.resource.name_idx < b.resource.name_idx;
	}
	friend bool operator > (const Construct &a, const Construct &b)
	{
		return a.resource.name_idx > b.resource.name_idx;
	}

	uint8_t num_values() const { return resource.fld_count; }
	qbrt_value & value(uint8_t i) { return fields[i]; }
	const qbrt_value & value(uint8_t i) const { return fields[i]; }
};

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
	uint8_t argc;

	Type(uint8_t id);
	Type(const std::string &mod, const std::string &name, uint8_t argc);

	static int compare(const Type &a, const Type &b)
	{
		if (a.id < b.id) {
			return -1;
		}
		if (a.id > b.id) {
			return +1;
		}
		if (a.module < b.module) {
			return -1;
		}
		if (a.module > b.module) {
			return +1;
		}
		if (a.name < b.name) {
			return -1;
		}
		if (a.name > b.name) {
			return +1;
		}
		return 0;
	}
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
