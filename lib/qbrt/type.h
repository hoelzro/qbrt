#ifndef QBRT_TYPE_H
#define QBRT_TYPE_H

#include "qbrt/core.h"
#include <cstdio>
#include <list>

class Module;


struct ParamResource
{
private:
	uint16_t _name_idx;	// String index
	uint16_t _type_idx;	// TypeSpecResource index

public:
	uint16_t name_idx() const { return be16toh(_name_idx); }
	uint16_t type_idx() const { return be16toh(_type_idx); }
};

struct TypeSpecResource
{
private:
	uint16_t _name_idx;	// type modsym
	uint16_t _fullname_idx;	// string
	uint16_t _args[];	// other TypeSpecResource indexes

public:
	uint16_t name_idx() const { return be16toh(_name_idx); }
	uint16_t fullname_idx() const { return be16toh(_fullname_idx); }
	uint16_t args(uint16_t i) const { return be16toh(_args[i]); }
};

struct DataTypeResource
{
private:
	uint16_t _name_idx;
	uint16_t _doc_idx;
	uint16_t _filename_idx;
	uint16_t _lineno;

public:
	uint8_t argc;

	uint16_t name_idx() const { return be16toh(_name_idx); }
	uint16_t doc_idx() const { return be16toh(_doc_idx); }
	uint16_t filename_idx() const { return be16toh(_filename_idx); }
	uint16_t lineno() const { return be16toh(_lineno); }

	static const uint32_t SIZE = 9;
};

struct ConstructResource
{
private:
	uint16_t _name_idx;
	uint16_t _doc_idx;
	uint16_t _filename_idx;
	uint16_t _lineno;
	uint16_t _datatype_idx;

public:
	uint8_t fld_count;
	uint8_t reserved;
	ParamResource fields[];

	uint16_t name_idx() const { return be16toh(_name_idx); }
	uint16_t doc_idx() const { return be16toh(_doc_idx); }
	uint16_t filename_idx() const { return be16toh(_filename_idx); }
	uint16_t lineno() const { return be16toh(_lineno); }
	uint16_t datatype_idx() const { return be16toh(_datatype_idx); }

	static const uint32_t SIZE = 12;
};

struct Construct
: public qbrt_value_index
{
	const Module &mod;
	const ConstructResource &resource;
	qbrt_value *fields;

	Construct(const Module &m, const ConstructResource &cr)
	: mod(m)
	, resource(cr)
	, fields(new qbrt_value[cr.fld_count])
	{}
	~Construct()
	{
		delete[] fields;
	}

	friend bool operator < (const Construct &a, const Construct &b)
	{
		return Construct::compare(a, b) < 0;
	}
	friend bool operator > (const Construct &a, const Construct &b)
	{
		return Construct::compare(a, b) > 0;
	}
	friend bool operator == (const Construct &a, const Construct &b)
	{
		return Construct::compare(a, b) == 0;
	}

	uint8_t num_values() const { return resource.fld_count; }
	qbrt_value & value(uint8_t i) { return fields[i]; }
	const qbrt_value & value(uint8_t i) const { return fields[i]; }

	const char * name() const;
	const DataTypeResource * datatype() const;
	static bool compare(const Construct &, const Construct &);
};

void load_construct_value_types(std::ostringstream &, const Construct &);


struct Tuple
: public qbrt_value_index
{
	qbrt_value *data;
	uint8_t size;

	Tuple(uint8_t sz)
		: data(new qbrt_value[sz])
		, size(sz)
	{}

	~Tuple()
	{
		delete[] data;
	}

	uint8_t num_values() const { return size; }
	qbrt_value & value(uint8_t i) { return data[i]; }
	const qbrt_value & value(uint8_t i) const { return data[i]; }
};


/** Static class for operating on list.List constructs */
struct List
{
	static void push(qbrt_value &head, const qbrt_value &item);
	static void reverse(qbrt_value &result, const qbrt_value &head);
	static void head(qbrt_value &result, const qbrt_value &head);
	static void pop(qbrt_value &result, const qbrt_value &head);
	static void is_empty(qbrt_value &result, const qbrt_value &head);
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
	const TaskID promiser;

	Promise(TaskID tid);
	~Promise();

	void mark_to_notify(bool &);
	void notify();

private:
	std::list< bool * > waiters;
	pthread_spinlock_t lock;
};

#endif
