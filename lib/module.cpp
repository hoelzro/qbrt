#include "qbrt/core.h"
#include "qbrt/module.h"
#include "qbrt/function.h"
#include "qbrt/type.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;


ObjectHeader::ObjectHeader()
: qbrt_version(0)
, flags(OFLAG_APP)
, name(0)
, version(0)
, iteration(0)
, imports(0)
, source_filename(0)
{
	magic[0] = 'q';
	magic[1] = 'b';
	magic[2] = 'r';
	magic[3] = 't';
}

bool operator < (const PolymorphArg &a, const PolymorphArg &b)
{
	// this is totally not the right thing
	return a.module < b.module;
}

uint32_t ResourceTable::offset(uint16_t i) const
{
	const ResourceInfo *info;
	info = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
	return info->offset() + ResourceTable::DATA_OFFSET;
}

uint32_t ResourceTable::size(uint16_t i) const
{
	const ResourceInfo *info1;
	info1 = (const ResourceInfo *) (index + i * ResourceInfo::SIZE);
	if (i + 1 >= resource_count) {
		return data_size - info1->offset();
	}
	const ResourceInfo *info2;
	info2 = (const ResourceInfo *)
		(index + (i + 1) * ResourceInfo::SIZE);
	return info2->offset() - info1->offset();
}

void add_type(Module &mod, const std::string &name, const Type &t)
{
	mod.types[name] = &t;
}

CFunction * add_c_function(Module &mod, c_function f, const std::string &name
		, uint8_t argc, const std::string &param_types)
{
	multimap< string, CFunction >::iterator it =
		mod.cfunction.insert(pair< string, CFunction >(name
				, CFunction(f, &mod, name, argc, param_types)));
	return &it->second;
}

CFunction * add_c_override(Module &mod, c_function f
		, const std::string &protomod, const std::string &protoname
		, const std::string &name, uint8_t argc
		, const std::string &param_types)
{
	CFunction *cfunc = add_c_function(mod, f, name, argc, param_types);
	cfunc->set_protocol(protomod, protoname);
}

const QbrtFunction * Module::fetch_function(const std::string &name) const
{
	const ResourceTable &tbl(resource);
	const FunctionHeader *f;
	const char *fname;
	for (uint16_t i(0); i<tbl.resource_count; ++i) {
		if (tbl.type(i) != RESOURCE_FUNCTION) {
			continue;
		}
		tbl.ptr(f, i);
		fname = fetch_string(tbl, f->name_idx());
		if (name != fname) {
			continue;
		}
		return qbrt_function(f);
	}
	return NULL;
}


struct ProtocolSearch
{
	typedef ProtocolResource resource_t;
	static const uint16_t resource_type_id = RESOURCE_PROTOCOL;

	int compare(const ResourceTable &tbl, uint16_t i) const
	{
		const ProtocolResource *proto = tbl.ptr< ProtocolResource >(i);
		const char *pname = fetch_string(tbl, proto->name_idx());
		if (name < pname) {
			return -1;
		}
		if (name > pname) {
			return 1;
		}
		return 0;
	}

	const std::string name;
	ProtocolSearch(const std::string &name)
		: name(name)
	{}
	~ProtocolSearch() {}
};

const ProtocolResource * Module::fetch_protocol(const std::string &name) const
{
	return resource.find(ProtocolSearch(name));
}

struct ProtocolFunctionSearch
{
	typedef FunctionHeader resource_t;
	static const uint16_t resource_type_id = RESOURCE_FUNCTION;

	/**
	 * Compare protocol & function to the function at i
	 */
	int compare(const ResourceTable &tbl, uint16_t i) const
	{
		const FunctionHeader *f = tbl.ptr< FunctionHeader >(i);
		uint8_t fct(PFC_TYPE(f->fcontext));
		if (FCT_PROTOCOL < fct) {
			return -1;
		}
		if (FCT_PROTOCOL > fct) {
			return 1;
		}

		const ProtocolResource *proto;
		proto = tbl.ptr< ProtocolResource >(f->context_idx());
		const char *pname = fetch_string(tbl, proto->name_idx());
		if (protocol < pname) {
			return -1;
		}
		if (protocol > pname) {
			return 1;
		}

		const char *fname = fetch_string(tbl, f->name_idx());
		if (function < fname) {
			return -1;
		}
		if (function > fname) {
			return 1;
		}
		return 0;
	}

	const std::string protocol;
	const std::string function;
	ProtocolFunctionSearch(const std::string &p, const std::string &f)
		: protocol(p)
		, function(f)
	{}
	~ProtocolFunctionSearch() {}
};

struct PolymorphFunctionSearch
{
	typedef FunctionHeader resource_t;
	static const uint16_t resource_type_id = RESOURCE_FUNCTION;

	/**
	 * Compare protocol & function to the function at i
	 */
	int compare(const ResourceTable &tbl, uint16_t i) const
	{
		const FunctionHeader *f = tbl.ptr< FunctionHeader >(i);
		if (PFC_OVERRIDE < f->fcontext) {
			cerr << "wtf is this fcontext? " << f->fcontext
				<< endl;
			return 0;
		}
		if (PFC_OVERRIDE > f->fcontext) {
			return 1;
		}

		const PolymorphResource *poly;
		poly = tbl.ptr< PolymorphResource >(f->context_idx());
		const ModSym &protoms(fetch_modsym(tbl, poly->protocol_idx()));
		// compare protocol module
		const char *other_mod = fetch_string(tbl, protoms.mod_name());
		if (this->proto_mod < other_mod) {
			return -1;
		}
		if (this->proto_mod > other_mod) {
			return 1;
		}
		// compare protocol symbol
		const char *other_sym = fetch_string(tbl, protoms.sym_name());
		if (this->proto_name < other_sym) {
			return -1;
		}
		if (this->proto_name > other_sym) {
			return 1;
		}

		const char *fname = fetch_string(tbl, f->name_idx());
		if (function < fname) {
			return -1;
		}
		if (function > fname) {
			return 1;
		}

		const char *param_types =
				fetch_string(tbl, f->param_types_idx());
		return compare_types(value_types.c_str(), param_types);
	}

	static int compare_types(const char *values, const char *params)
	{
		int value_len(strlen(values));
		int param_len(strlen(params));
		int cmp_len(value_len < param_len ? value_len : param_len);
		int vi(0), pi(0);
		while (vi < value_len && pi < param_len) {
			if (values[vi] == params[pi]) {
				++vi;
				++pi;
				continue;
			}
			if (strncmp(params + pi, "*/", 2) == 0) {
				while (values[++vi] != ')');
				while (params[++pi] != ')');
				continue;
			}
			if (values[vi] < params[pi]) {
				return -1;
			}
			// values[vi] must be > params[pi]
			return 1;
		}

		int value_rem(value_len - vi);
		int param_rem(param_len - pi);
		if (value_rem < param_rem) {
			return -1;
		}
		if (value_rem > param_rem) {
			return +1;
		}
		return 0;
	}

	const std::string &proto_mod;
	const std::string &proto_name;
	const std::string &function;
	const std::string &value_types;

	PolymorphFunctionSearch(const std::string &pmod
			, const std::string &pname, const std::string &f
			, const string &value_types)
		: proto_mod(pmod)
		, proto_name(pname)
		, function(f)
		, value_types(value_types)
	{}
};

struct ConstructSearch
{
	const std::string &name;

	ConstructSearch(const std::string &name)
	: name(name)
	{}

	typedef ConstructResource resource_t;
	static const uint16_t resource_type_id = RESOURCE_CONSTRUCT;

	/**
	 * Compare name to the construct at i
	 */
	int compare(const ResourceTable &tbl, uint16_t i) const
	{
		const ConstructResource *i_cons;
		i_cons = tbl.ptr< ConstructResource >(i);
		const char *i_name = fetch_string(tbl, i_cons->name_idx());

		if (name < i_name) {
			return -1;
		} else if (name > i_name) {
			return +1;
		}
		return 0;
	}
};

struct DataTypeSearch
{
	const std::string &name;

	DataTypeSearch(const std::string &name)
	: name(name)
	{}

	typedef DataTypeResource resource_t;
	static const uint16_t resource_type_id = RESOURCE_DATATYPE;

	/**
	 * Compare name to the datatype at i
	 */
	int compare(const ResourceTable &tbl, uint16_t i) const
	{
		const DataTypeResource *i_dtr;
		i_dtr = tbl.ptr< DataTypeResource >(i);
		const char *i_name = fetch_string(tbl, i_dtr->name_idx);

		if (name < i_name) {
			return -1;
		} else if (name > i_name) {
			return +1;
		}
		return 0;
	}
};

const QbrtFunction * Module::fetch_protocol_function(
		const std::string &protoname
		, const std::string &fname) const
{
	const FunctionHeader *f;
	f = resource.find(ProtocolFunctionSearch(protoname, fname));
	if (!f) {
		return NULL;
	}
	return qbrt_function(f);
}

const QbrtFunction * Module::fetch_override(const string &protomod
		, const string &protoname, const string &fname
		, const string &value_types) const
{
	const FunctionHeader *f;
	PolymorphFunctionSearch query(protomod, protoname, fname, value_types);
	f = resource.find(query);
	if (!f) {
		return NULL;
	}
	return qbrt_function(f);
}

const Type * indexed_datatype(const Module &mod, uint16_t idx)
{
	map< uint16_t, const Type * >::const_iterator it;
	it = mod.indexed_type_cache.find(idx);
	if (it != mod.indexed_type_cache.end()) {
		return it->second;
	}

	const DataTypeResource *dtr;
	dtr = mod.resource.ptr< DataTypeResource >(idx);
	if (!dtr) {
		cerr << "DataTypeResource not found at: " << idx << endl;
		return NULL;
	}
	const char *name = fetch_string(mod.resource, dtr->name_idx);
	const Type *t = new Type(mod.name, name, dtr->argc);
	mod.indexed_type_cache[idx] = t;
	return t;
}

const QbrtFunction * Module::qbrt_function(const FunctionHeader *f) const
{
	const QbrtFunction *result = function_cache[f];
	if (!result) {
		result = new QbrtFunction(f, this);
		function_cache[f] = result;
	}
	return result;
}

bool open_qb(ifstream &objstr, const std::string &objname)
{
	const char *envpath(getenv("QBPATH"));
	if (!(envpath && *envpath)) {
		envpath = ".";
	}
	char *path(strdup(envpath));

	const string qbname(objname +".qb");
	const char *dir = strtok(path, ":");
	char truedir[4096];
	while (dir) {
		realpath(dir, truedir);
		string filename(truedir);
		filename += "/" + qbname;
		objstr.open(filename.c_str(), ios::in | ios::binary);
		if (objstr) {
			return true;
		}
		dir = strtok(NULL, ":");
	}
	free(path);
	cerr << "failed to find " << qbname << " in QBPATH=" << envpath << endl;
	return false;
}

void read_header(ObjectHeader &h, istream &input)
{
	input.read((char *) &h, ObjectHeader::SIZE);
	h.qbrt_version = be32toh(h.qbrt_version);
	h.flags = be64toh(h.flags);
	h.name = be16toh(h.name);
	h.version = be16toh(h.version);
	h.iteration = be16toh(h.iteration);
	h.imports = be16toh(h.imports);
	h.source_filename = be16toh(h.source_filename);
}

void read_resource_table(ResourceTable &tbl, istream &input)
{
	tbl.data_size = read32(input);
	tbl.resource_count = read16(input);
	uint32_t index_size(tbl.resource_count * ResourceInfo::SIZE);

	uint8_t *data = new uint8_t[tbl.data_size];
	uint8_t *index = new uint8_t[index_size];

	input.read((char *) data, tbl.data_size);
	input.read((char *) index, index_size);

	tbl.data = data;
	tbl.index = index;
}

Module * read_module(const string &objname)
{
	ifstream in;
	if (!open_qb(in, objname)) {
		return NULL;
	}
	Module *mod = new Module(objname);
	read_header(mod->header, in);
	read_resource_table(mod->resource, in);
	in.close();
	if (mod->header.name == 0) {
		cerr << "module name is not set for: " << objname & DIE;
	}
	const char *header_name(fetch_string(mod->resource, mod->header.name));
	if (header_name != objname) {
		cerr << "module name mismatch: " << mod->header.name << "/"
			<< (int) *header_name << " != " << objname & DIE;
	}

	return mod;
}

const CFunction * fetch_c_function(const Module &m, const std::string &name)
{
	pair< multimap< string, CFunction >::const_iterator
		, multimap< string, CFunction >::const_iterator > range;
	range = m.cfunction.equal_range(name);
	if (range.first == range.second) {
		return NULL;
	}
	multimap< string, CFunction >::const_iterator it(range.first);
	for (; it != range.second; ++it) {
		if (it->second.fcontext() == PFC_NONE) {
			// there should be only 1 regular function with
			// this name. looks like we found it.
			return &it->second;
		}
	}

	return NULL;
}

const CFunction * fetch_c_override(const Module &m, const std::string &protomod
		, const std::string &protoname, const std::string &name
		, const std::string &value_types)
{
	pair< multimap< string, CFunction >::const_iterator
		, multimap< string, CFunction >::const_iterator > range;
	range = m.cfunction.equal_range(name);
	if (range.first == range.second) {
		return NULL;
	}
	multimap< string, CFunction >::const_iterator it(range.first);
	for (; it != range.second; ++it) {
		if (it->second.proto_module != protomod) {
			continue;
		}
		if (it->second.proto_name != protoname) {
			continue;
		}
		if (it->second.param_types != value_types) {
			continue;
		}
		return &it->second;
	}
	return NULL;
}

const ConstructResource * find_construct(const Module &m
		, const std::string &name)
{
	ConstructSearch query(name);
	const ConstructResource *cons;
	cons = m.resource.find(query);
	if (!cons) {
		return NULL;
	}
	return cons;
}

void Module::load_construct(qbrt_value &dst, const Module &m, const char *name)
{
	const ConstructResource *construct_r;
	construct_r = find_construct(m, name);

	const Type *typ = indexed_datatype(m, construct_r->datatype_idx());

	Construct *cons = new Construct(m, *construct_r);
	qbrt_value::construct(dst, typ, cons);
}
