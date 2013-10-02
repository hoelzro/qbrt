#ifndef QBRT_MODULE_H
#define QBRT_MODULE_H

#include "qbrt/resourcetype.h"
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <stack>
#include "string.h"
#include "function.h"


struct Module;

#pragma pack(push, 1)

#define SET_OFLAG(flags, bit, value) \
	do { flags = value ? ( flags | bit) : (flags & (~ bit)); } while (0);

#define OFLAG_APP (((uint64_t) 1) << 63)

struct ObjectHeader
{
	char magic[4];
	uint32_t qbrt_version;
	uint64_t flags;
	uint16_t name;
	uint16_t version;
	uint16_t iteration;
	uint16_t imports;
	uint16_t source_filename;

	ObjectHeader();

	bool application() const { flags & OFLAG_APP; }
	void set_application(bool app) { SET_OFLAG(flags, OFLAG_APP, app); }

	static const uint32_t SIZE = 26;
};

struct ResourceInfo
{
	uint32_t _offset;
	uint16_t _type;

	uint32_t offset() const { return be32toh(_offset); }
	uint32_t type() const { return be16toh(_type); }

	ResourceInfo(uint32_t offset, uint16_t typ)
	: _offset(offset)
	, _type(typ)
	{}

	static const uint32_t SIZE = 6;
};

struct ModSym
{
private:
	uint16_t _mod_name;
	uint16_t _sym_name;

public:
	uint16_t mod_name() const { return be16toh(_mod_name); }
	uint16_t sym_name() const { return be16toh(_sym_name); }

	typedef std::vector< ModSym * > Array;
};

struct FullId
{
	const char *module;
	const char *id;

	FullId(const char *mod, const char *id)
	: module(mod)
	, id(id)
	{}
};

struct ImportResource
{
private:
	uint16_t _count;
	uint16_t _modules[];

public:
	uint16_t count() const { return be16toh(_count); }
	uint16_t modules(uint16_t i) const { return be16toh(_modules[i]); }
};


struct ResourceTableHeader
{
	// converted to host byte order when it's read
	uint32_t data_size;
	uint16_t resource_count;

	ResourceTableHeader()
		: data_size(0)
		, resource_count(0)
	{}

	static const uint32_t SIZE = 6;
};

#pragma pack(pop)


struct ResourceTable
{
	static const uint32_t DATA_OFFSET =
		ObjectHeader::SIZE + ResourceTableHeader::SIZE;

	uint16_t type(uint16_t i) const
	{
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		return info->type();
	}

	template < typename D >
	const D * ptr(uint16_t i) const
	{
		if (i == 0) {
			std::cerr << "resource id: zero" & DIE;
		}
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		return (const D *) (data + info->offset());
	}
	template < typename D >
	void ptr(const D *&p, uint16_t i) const
	{
		if (i == 0) {
			std::cerr << "resource id: zero" & DIE;
		}
		const ResourceInfo *info;
		info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
		p = (const D *) (data + info->offset());
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
	const ProtocolResource * fetch_protocol(const std::string &name) const;
	const QbrtFunction * fetch_protocol_function(
			const std::string &protocol_name
			, const std::string &function_name) const;

	Module(const std::string &module_name)
	: name(module_name)
	{}

	friend void add_type(Module &, const std::string &name, const Type &);
	friend const Type * indexed_datatype(const Module &, uint16_t idx);

	static void load_construct(qbrt_value &, const Module &
			, const char *name);

private:
	const QbrtFunction * qbrt_function(const FunctionHeader *) const;
	mutable std::map< const FunctionHeader *, const QbrtFunction * >
		function_cache;
	mutable std::map< uint16_t, const Type * > indexed_type_cache;
};
typedef std::map< std::string, const Module * > ModuleMap;


Module * read_module(const std::string &objname);

const CFunction * fetch_c_function(const Module &, const std::string &name);
const CFunction * fetch_c_override(const Module &, const std::string &protomod
		, const std::string &protoname, const std::string &name
		, const std::string &param_types);

const ConstructResource * find_construct(const Module &
		, const std::string &name);

static inline const char * fetch_string(const ResourceTable &tbl, uint16_t idx)
{
	const StringResource *res = tbl.ptr< StringResource >(idx);
	return res ? res->value : NULL;
}

static inline const ModSym & fetch_modsym(const ResourceTable &tbl, uint16_t i)
{
	return tbl.obj< ModSym >(i);
}

static inline FullId fetch_fullid(const ResourceTable &tbl, uint16_t i)
{
	const ModSym &msr(fetch_modsym(tbl, i));
	const char *mod(fetch_string(tbl, msr.mod_name()));
	const char *id(fetch_string(tbl, msr.sym_name()));
	return FullId(mod, id);
}

static inline const TypeSpecResource & fetch_typespec(const ResourceTable &tbl
		, uint16_t idx)
{
	return tbl.obj< TypeSpecResource >(idx);
}


CFunction * add_c_function(Module &, c_function
		, const std::string &name, uint8_t argc
		, const std::string &param_types);
CFunction * add_c_override(Module &, c_function
		, const std::string &protomod, const std::string &protoname
		, const std::string &name, uint8_t argc
		, const std::string &param_types);

static inline uint16_t read16(std::istream &in)
{
	uint16_t result;
	in.read((char *) &result, 2);
	return be16toh(result);
}
static inline uint32_t read32(std::istream &in)
{
	uint32_t result;
	in.read((char *) &result, 4);
	return be32toh(result);
}

bool open_qb(std::ifstream &lib, const std::string &qbname);

#endif
