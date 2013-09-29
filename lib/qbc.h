#ifndef QBRT_ASM_H
#define QBRT_ASM_H

#include "qbrt/core.h"
#include "qbrt/stmt.h"
#include <iostream>
#include <string>
#include <list>
#include <set>


struct AsmReg;
struct AsmDataType;

struct RegAlloc
{
	typedef std::map< std::string, uint8_t > CountMap;

	CountMap registry;
	const uint8_t argc;
	uint8_t counter;

	RegAlloc(uint8_t argc)
	: argc(argc)
	, counter(0)
	{}
	void declare_arg(const std::string &name, const std::string &type);
	void assign_src(AsmReg &);
	void alloc_dst(AsmReg &, const std::string &type = std::string());
};


struct ResourceLess
{
	bool operator () (const AsmResource *, const AsmResource *) const;
};

struct AsmImport
: public AsmResource
{
	std::set< AsmString *, ResourceLess > modules;
	std::set< std::string > names;

	AsmImport() : AsmResource(RESOURCE_IMPORT) {}
	virtual uint32_t write(std::ostream &o) const;
	virtual std::ostream & pretty(std::ostream &o) const;
};

struct ResourceSet
{
	typedef std::map< AsmResource *, uint16_t, ResourceLess > Indexer;
	typedef Indexer::iterator iterator;
	typedef Indexer::const_iterator const_iterator;

	uint16_t module_name() const { return *_module_name.index; }
	uint16_t module_version;
	uint16_t module_iteration() const { return *_module_iteration.index; }
	uint16_t imports_index() const { return *imports.index; }

	ResourceSet(const std::string &modname);

	iterator begin() { return index.begin(); }
	iterator end() { return index.end(); }
	const_iterator begin() const { return index.begin(); }
	const_iterator end() const { return index.end(); }
	uint16_t size() const { return index.size(); }

	bool collect(AsmResource &);
	void import(AsmString &);
	void set_module_version(uint16_t ver, const std::string &iter);

	const std::set< std::string > & imported_modules() const
		{ return imports.names; }

private:
	Indexer index;
	AsmImport imports;
	AsmString _module_name;
	AsmString _module_iteration;
};


struct AsmReg
{
	std::string name;
	std::string sub_name;
	std::string data_type;
	reg_t specialid;
	const char reg_type;
	int8_t idx;
	int8_t ext;

	AsmReg(char typec)
	: name()
	, reg_type(typec)
	, specialid(0)
	, idx(-1)
	, ext(-1)
	{}

	operator reg_t () const;

	friend std::ostream & operator << (std::ostream &, const AsmReg &);

	static AsmReg * var(const std::string &name);
	static AsmReg * arg(const std::string &name);
	void parse_subindex(const std::string &idx);

	static inline AsmReg * create_void()
	{
		AsmReg *r = new AsmReg('c');
		r->specialid = CONST_REG_VOID;
		return r;
	}
	static AsmReg * create_special(uint16_t id);
	static inline AsmReg * create_result()
	{
		return create_special(REG_RESULT);
	}
};

struct AsmModSym
: public AsmResource
{
	AsmString module;
	AsmString symbol;

	AsmModSym(const std::string &modname, const std::string &symname)
	: AsmResource(RESOURCE_MODSYM)
	, module(modname)
	, symbol(symname)
	{}

	virtual uint32_t write(std::ostream &) const;

	virtual std::ostream & pretty(std::ostream &o) const
	{
		return o << "modsym:" << *this;
	}

	friend inline std::ostream & operator << (
			std::ostream &o, const AsmModSym &ms)
	{
		return o << ms.module.value << '/' << ms.symbol.value;
	}

	friend int compare_asm(const AsmModSym &, const AsmModSym &);
};


struct AsmTypeSpec
: public AsmResource
{
	AsmModSym *name;
	AsmTypeSpecList *args;
	mutable AsmString fullname;

	AsmTypeSpec(AsmModSym *name)
	: AsmResource(RESOURCE_TYPESPEC)
	, name(name)
	, args(NULL)
	{}

	bool empty() const { return !args || args->empty(); }
	int argc() const { return args ? args->size() : 0; }
	uint32_t write(std::ostream &o) const;
	std::ostream & serialize(std::ostream &o) const;
	std::ostream & pretty(std::ostream &o) const;

	friend inline std::ostream & operator << (
			std::ostream &o, const AsmTypeSpec &t)
	{
		return t.pretty(o);
	}
};


struct AsmProtocol
: public AsmResource
{
	const AsmString &name;
	AsmString doc;
	std::list< AsmString * > *typevar;
	uint16_t line_no;
	uint16_t argc;

	AsmProtocol(const AsmString &name, std::list< AsmString * > *types)
		: AsmResource(RESOURCE_PROTOCOL)
		, name(name)
		, typevar(types)
		, line_no(0)
		, argc(types->size())
	{}

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &o) const
	{
		o << "protocol:" << name.value;
		return o;
	}
};

struct AsmPolymorph
: public AsmResource
{
	const AsmModSym &protocol;
	AsmString doc;
	uint16_t line_no;
	AsmTypeSpecList type;

	AsmPolymorph(const AsmModSym &proto)
		: AsmResource(RESOURCE_POLYMORPH)
		, protocol(proto)
		, line_no(0)
	{}

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &o) const;
};

struct AsmConstruct
: public AsmResource
{
	const AsmString &name;
	const AsmResource &datatype;
	AsmString doc;
	AsmString filename;
	uint16_t line_no;
	AsmParamList fields;

	AsmConstruct(const AsmString &name, const AsmResource &datatype)
	: AsmResource(RESOURCE_CONSTRUCT)
	, name(name)
	, datatype(datatype)
	, line_no(0)
	{}

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &o) const;
};

struct AsmDataType
: public AsmResource
{
	const AsmString &name;
	AsmString doc;
	AsmString filename;
	uint16_t line_no;
	uint8_t argc;

	AsmDataType(const AsmString &name, uint8_t argc)
	: AsmResource(RESOURCE_DATATYPE)
	, name(name)
	, line_no(0)
	, argc(argc)
	{}

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &o) const;
};


template < typename T >
struct AsmCmp {
	static int cmp(const T &, const T &);
};

bool collect_resource(ResourceSet &, AsmResource &);
bool collect_string(ResourceSet &, AsmString &);
bool collect_modsym(ResourceSet &, AsmModSym &);
bool collect_typespec(ResourceSet &, AsmTypeSpec &);
void collect_resources(ResourceSet &, Stmt::List &);



typedef std::list< instruction * > codeblock;
typedef std::map< std::string, uint32_t > label_map;

struct jump_instruction;

struct jump_data
{
	jump_instruction &ji;
	std::string label;
	uint32_t jump_offset;

	jump_data(jump_instruction &j, const std::string &lbl
			, uint32_t joffset)
		: ji(j)
		, label(lbl)
		, jump_offset(joffset)
	{}
};

struct AsmParam
{
	AsmParam(const AsmString &name, const AsmTypeSpec &typ)
	: name(name)
	, type(typ)
	{}

	const AsmString &name;
	const AsmTypeSpec &type;
};

struct AsmFunc
: public AsmResource
{
	Stmt::List *stmts;
	codeblock code;
	label_map label;
	std::list< jump_data > jump;
	AsmResource *ctx;
	AsmParamList params;
	AsmString param_types;
	const AsmTypeSpec &result_type;
	const AsmString &name;
	AsmString doc;
	uint16_t line_no;
	uint8_t fcontext;
	uint8_t argc;
	uint8_t regc;

	AsmFunc(const AsmString &name, const AsmTypeSpec &result);

	bool has_code() const;
	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &) const;
};

struct DeclList
{
	std::list< AsmFunc * > func;
	// std::list< AsmProtocol * > proto;
};

void set_function_context(Stmt::List &, uint8_t asmfc, AsmResource *);
void allocate_registers(Stmt::List &, RegAlloc *);

void label_next(AsmFunc &, const std::string &lbl);
void asm_jump(AsmFunc &, const std::string &lbl, jump_instruction *);
void asm_instruction(AsmFunc &, instruction *);
void generate_codeblock(AsmFunc &, const Stmt::List &);

#endif
