#ifndef QBRT_MODULE_H
#define QBRT_MODULE_H

#include "qbrt/resourcetype.h"
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <stack>
#include <string.h>
#include "string.h"
#include "function.h"


struct Module;

struct Version
{
	uint16_t version;
	char iteration[6];
};

union ObjectFlags
{
	uint64_t raw;
	struct {
		uint64_t application : 1;
		uint64_t reserved : 63;
	} f;
};

struct ObjectHeader
{
	char magic[4];
	uint32_t qbrt_version;
	ObjectFlags flags;
	char library_name[24];
	uint16_t library_version;
	char library_iteration[6];

	ObjectHeader()
		: qbrt_version(0)
		, library_version(0)
	{
		magic[0] = 'q';
		magic[1] = 'b';
		magic[2] = 'r';
		magic[3] = 't';
		flags.raw = 0;
		flags.f.application = 1;
		memset(library_name, '\0', sizeof(library_name));
		memset(library_iteration, '\0', sizeof(library_iteration));
	}

	static const uint32_t SIZE = 48;
};

struct ResourceName
{
	uint16_t name_id;
	uint16_t resource_id;

	ResourceName(uint16_t name, uint16_t id)
		: name_id(name)
		, resource_id(id)
	{}

	static const uint32_t SIZE = 4;
};

struct ResourceInfo
{
	uint32_t offset;
	uint16_t type;

	ResourceInfo(uint32_t offset, uint16_t typ)
		: offset(offset)
		, type(typ)
	{}

	static const uint32_t SIZE = 6;
};

struct ModSym
{
	uint16_t mod_name;
	uint16_t sym_name;

	typedef std::vector< ModSym * > Array;
};


struct ResourceTableHeader
{
	uint32_t data_size;
	uint16_t resource_count;

	ResourceTableHeader()
		: data_size(0)
		, resource_count(0)
	{}

	static const uint32_t SIZE = 8;
};

struct ResourceTable
{
	static const uint32_t DATA_OFFSET =
		ObjectHeader::SIZE + ResourceTableHeader::SIZE;

	uint16_t type(uint16_t i) const
	{
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		return info->type;
	}

	template < typename D >
	const D * ptr(uint16_t i) const
	{
		if (i == 0) {
			return NULL;
		}
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		return (const D *) (data + info->offset);
	}
	template < typename D >
	void ptr(const D *&p, uint16_t i) const
	{
		if (i == 0) {
			p = NULL;
			return;
		}
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		p = (const D *) (data + info->offset);
	}
	template < typename D >
	const D & obj(uint16_t i) const
	{
		return *(ptr< D >(i));
	}

	/**
	 * Compare a key against a given resource to see if it matches
	 */
	template < typename C >
	int compare(const C &cmp, uint16_t i) const
	{
		uint16_t rtype(type(i));
		if (cmp.resource_type_id < rtype) {
			return -1;
		}
		if (cmp.resource_type_id > rtype) {
			return 1;
		}
		return cmp.compare(*this, i);
	}
	/**
	 * Find a particular type of resource for a given key
	 */
	template < typename C >
	const typename C::resource_t *
	find(const C &cmp) const
	{
		uint16_t i(1);
		uint16_t rtype;
		for (; i<resource_count; ++i) {
			int comparison(compare(cmp, i));
			if (comparison == 0) {
				return ptr< typename C::resource_t >(i);
			}
		}
		return NULL;
	}

	/**
	 * Return the file offset of the given resource
	 */
	uint32_t offset(uint16_t i) const;

	/**
	 * Return the size of the given resource
	 */
	uint32_t size(uint16_t i) const;

	uint32_t index_offset() const
	{ return ResourceTable::DATA_OFFSET + this->data_size; }

	const uint8_t *data;
	const uint8_t *index;
	uint32_t data_size;
	uint16_t resource_count;
};

struct Module
{
	std::string name;
	ObjectHeader header;
	ResourceTable resource;
	std::map< std::string, const Type * > types;
	std::multimap< std::string, CFunction > cfunction;

	const void * fetch_resource(const std::string &name) const;
	const QbrtFunction * fetch_function(const std::string &name) const;
	const QbrtFunction * fetch_override(const std::string &protomod
		, const std::string &protoname, const std::string &fname
		, const std::string &param_types) const;
	const Type * fetch_struct(const std::string &name) const;
	const ProtocolResource * fetch_protocol(const std::string &name) const;
	const QbrtFunction * fetch_protocol_function(
			const std::string &protocol_name
			, const std::string &function_name) const;

	Module(const std::string &module_name)
	: name(module_name)
	{}

private:
	const QbrtFunction * qbrt_function(const FunctionHeader *) const;
	mutable std::map< const FunctionHeader *, const QbrtFunction * >
		function_cache;
};

Module * read_module(const std::string &objname);

const CFunction * fetch_c_function(const Module &, const std::string &name);
const CFunction * fetch_c_override(const Module &, const std::string &protomod
		, const std::string &protoname, const std::string &name
		, const std::string &param_types);

static inline const char * fetch_string(const ResourceTable &tbl, uint16_t idx)
{
	const StringResource *res = tbl.ptr< StringResource >(idx);
	return res ? res->value : NULL;
}

static inline const ModSym & fetch_modsym(const ResourceTable &tbl, uint16_t i)
{
	return tbl.obj< ModSym >(i);
}


void add_type(Module &, const std::string &name, const Type &);
CFunction * add_c_function(Module &, c_function
		, const std::string &name, uint8_t argc
		, const std::string &param_types);
CFunction * add_c_override(Module &, c_function
		, const std::string &protomod, const std::string &protoname
		, const std::string &name, uint8_t argc
		, const std::string &param_types);

bool open_qb(std::ifstream &lib, const std::string &qbname);
		// , const Version &min, const Version &max);

#endif
