#ifndef QBRT_ASM_H
#define QBRT_ASM_H

#include "qbrt/core.h"
#include "qbrt/stmt.h"
#include <iostream>
#include <string>
#include <list>


struct AsmReg;

struct RegAlloc
{
	typedef std::map< std::string, int > CountMap;

	CountMap args;
	CountMap registry;
	int counter;

	RegAlloc(int argc)
	: counter(argc)
	{}
	void alloc(AsmReg &);

// private-ish
	void alloc_arg(AsmReg &);
	void alloc_reg(AsmReg &);
};



struct AsmReg
{
	std::string name;
	reg_t specialid;
	int8_t idx;
	int8_t ext;
	const char type;

	AsmReg(char typec)
	: name()
	, specialid(0)
	, idx(-1)
	, ext(-1)
	, type(typec)
	{}

	operator reg_t () const;

	friend std::ostream & operator << (std::ostream &, const AsmReg &);

	static AsmReg * parse_arg(const std::string &arg);
	static AsmReg * parse_reg(const std::string &reg);
	static AsmReg * parse_extended_arg(const std::string &arg
			, const std::string &ext);
	static AsmReg * parse_extended_reg(const std::string &reg
			, const std::string &ext);
	static int parse_regext(const std::string &ext);
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

	virtual uint32_t write(std::ostream &o) const
	{
		o.write((const char *) module.index, 2);
		o.write((const char *) symbol.index, 2);
		return 4;
	}

	virtual std::ostream & pretty(std::ostream &o) const
	{
		return o << *this;
	}

	friend inline std::ostream & operator << (
			std::ostream &o, const AsmModSym &ms)
	{
		o << "modsym:" << ms.module.value << ms.symbol.value;
		return o;
	}

	friend int compare_asm(const AsmModSym &, const AsmModSym &);
};


struct AsmFunc;

struct AsmProtocol
: public AsmResource
{
	const AsmString &name;
	AsmString doc;
	AsmString filename;
	AsmString *typevar;
	uint16_t line_no;
	uint16_t argc;

	AsmProtocol(const AsmString &name)
		: AsmResource(RESOURCE_PROTOCOL)
		, name(name)
		, typevar(NULL)
		, line_no(0)
		, argc(0)
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
	AsmString filename;
	uint16_t line_no;
	AsmModSymList type;

	AsmPolymorph(const AsmModSym &proto)
		: AsmResource(RESOURCE_POLYMORPH)
		, protocol(proto)
		, line_no(0)
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
	AsmParam(const AsmString &name, const AsmModSym &typ)
	: name(name)
	, type(typ)
	{}

	const AsmString &name;
	const AsmModSym &type;
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
	const AsmString &name;
	AsmString doc;
	AsmString filename;
	uint16_t line_no;
	uint8_t fcontext;
	uint8_t argc;
	uint8_t regc;

	AsmFunc(const AsmString &name);

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
