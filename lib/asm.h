#ifndef QBRT_ASM_H
#define QBRT_ASM_H

#include "qbrt/module.h"
#include <iostream>
#include <string>
#include <list>


struct AsmReg;

struct RegAlloc
{
	typedef std::map< std::string, int > CountMap;

	CountMap registry;
	int counter;

	RegAlloc(int argc)
	: counter(argc)
	{}
	void alloc(AsmReg &);
};


struct AsmResource;

struct ResourceLess
{
	bool operator () (const AsmResource *, const AsmResource *) const;
};

typedef std::map< AsmResource *, uint16_t, ResourceLess > ResourceSet;
static inline ResourceSet::value_type asm_resource_pair(AsmResource *r)
{
	return ResourceSet::value_type(r, 0);
}


struct AsmReg
{
	std::string name;
	int idx;
	int ext;
	const char type;

	AsmReg(char typec)
	: name()
	, idx(-1)
	, ext(-1)
	, type(typec)
	{}

	operator reg_t () const;

	friend std::ostream & operator << (std::ostream &o, const AsmReg &r)
	{
		o << r.type << r.idx;
		if (r.ext < 0) {
			o << '.' << r.ext;
		}
		return o;
	}

	static AsmReg * parse_arg(const std::string &arg);
	static AsmReg * parse_reg(const std::string &reg);
	static AsmReg * parse_extended_arg(const std::string &arg
			, const std::string &ext);
	static AsmReg * parse_extended_reg(const std::string &reg
			, const std::string &ext);
	static int parse_regext(const std::string &ext);
	static AsmReg * create_void();
	static AsmReg * create_result();
};

struct AsmResource
{
	uint16_t type;
	uint16_t *index;

	AsmResource(uint16_t typ)
		: type(typ)
		, index(&NULL_INDEX)
	{}
	virtual uint32_t write(std::ostream &) const = 0;
	virtual std::ostream & pretty(std::ostream &) const = 0;

	static uint16_t NULL_INDEX;
};

struct AsmString
: public AsmResource
{
	std::string value;

	AsmString()
		: AsmResource(RESOURCE_STRING)
		, value()
	{}
	AsmString(const std::string &val)
		: AsmResource(RESOURCE_STRING)
		, value(val)
	{}

	virtual uint32_t write(std::ostream &o) const
	{
		uint16_t strsize(value.size());
		o.write((const char *) &strsize, 2);
		o.write(value.c_str(), strsize);
		o.put('\0');
		return strsize + 3;
	}

	virtual std::ostream & pretty(std::ostream &o) const
	{
		o << "string:";
		std::string::const_iterator it;
		for (it=value.begin(); it!=value.end(); ++it) {
			switch (*it) {
				case '\n':
					o << "\\n";
					break;
				case '\t':
					o << "\\t";
					break;
				case '\"':
					o << "\\\"";
					break;
				default:
					o << *it;
					break;
			}
		}
		return o;
	}
};

struct AsmHashTag
: public AsmResource
{
	std::string value;

	AsmHashTag()
		: AsmResource(RESOURCE_HASHTAG)
		, value()
	{}
	AsmHashTag(const std::string &val)
		: AsmResource(RESOURCE_HASHTAG)
		, value(val)
	{}

	virtual uint32_t write(std::ostream &o) const
	{
		uint16_t strsize(value.size());
		o.write((const char *) &strsize, 2);
		o.write(value.c_str(), strsize);
		o.put('\0');
		return strsize + 3;
	}

	virtual std::ostream & pretty(std::ostream &o) const
	{
		o << "#" << value;
		return o;
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
		o << "modsym:" << ms.module.value << "." << ms.symbol.value;
		return o;
	}

	friend int compare_asm(const AsmModSym &, const AsmModSym &);
};
typedef std::list< AsmModSym * > AsmModSymList;


struct AsmLabel
{
	std::string name;

	AsmLabel(const std::string &lbl)
	: name(lbl)
	{} 
};

struct AsmFunc;

struct AsmProtocol
: public AsmResource
{
	const AsmString &name;
	AsmString doc;
	AsmString filename;
	uint16_t line_no;
	uint16_t argc;

	AsmProtocol(const AsmString &name)
		: AsmResource(RESOURCE_PROTOCOL)
		, name(name)
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


struct Stmt
{
	typedef std::list< Stmt * > List;

	virtual ~Stmt() {}

	virtual void set_function_context(uint8_t, AsmResource *) {}
	virtual void allocate_registers(RegAlloc *) {}
	virtual void collect_resources(ResourceSet &) {}
	virtual void generate_code(AsmFunc &)
		{ std::cout << "no generate_code()"; }
	virtual void pretty(std::ostream &) const = 0;
};

bool collect_resource(ResourceSet &, AsmResource &);
bool collect_string(ResourceSet &, AsmString &);
bool collect_modsym(ResourceSet &, AsmModSym &);
void collect_resources(ResourceSet &, Stmt::List &);


struct binaryop_stmt
: public Stmt
{
	binaryop_stmt(char op, char typ, AsmReg *result
			, AsmReg *a, AsmReg *b)
	: result(result)
	, a(a)
	, b(b)
	, op(op)
	, type(typ)
	{}
	AsmReg *result;
	AsmReg *a;
	AsmReg *b;
	char op;
	char type;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "binaryop " << op << type <<' '<< *result
				<<' '<< *a <<' '<< *b;
	}
};

struct brbool_stmt
: public Stmt
{
	brbool_stmt(bool check, AsmReg *reg, const std::string &lbl)
		: check(check)
		, label(lbl)
		, reg(reg)
	{}
	AsmLabel label;
	AsmReg *reg;
	bool check;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << (check ? "brt " : "brf ")
			<< *reg << " #" << label.name;
	}
};

struct brcmp_stmt
: public Stmt
{
	AsmReg *a;
	AsmReg *b;
	AsmLabel label;
	int8_t opcode;

	static brcmp_stmt * ne(AsmReg *, AsmReg *, const std::string &lbl);

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "brcmp " << *a << ' ' << *b << label.name;
	}

private:
	brcmp_stmt(uint8_t op, AsmReg *a, AsmReg *b
			, const std::string &lbl)
	: a(a)
	, b(b)
	, label(lbl)
	, opcode(op)
	{}
};

struct brfail_stmt
: public Stmt
{
	brfail_stmt(bool check, AsmReg *reg, const std::string &lbl)
	: check(check)
	, reg(reg)
	, label(lbl)
	{}
	AsmReg *reg;
	AsmLabel label;
	bool check;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << (check ? "brfail " : "brnfail ")
			<< *reg << " #" << label.name;
	}
};

struct call_stmt
: public Stmt
{
	call_stmt(AsmReg *result, AsmReg *func)
		: result(result)
		, function(func)
	{}
	AsmReg *result;
	AsmReg *function;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "call " << *result
			<< " " << *function;
	}
};

struct cfailure_stmt
: public Stmt
{
	cfailure_stmt(const std::string &type)
	: type(type)
	{}

	AsmHashTag type;

	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "cfailure #" << type.value;
	}
};

struct consti_stmt
: public Stmt
{
	consti_stmt(AsmReg *dst, int32_t val)
		: dst(dst)
		, value(val)
	{}
	int32_t value;
	AsmReg *dst;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "consti " << *dst << " " << value;
	}
};

struct consts_stmt
: public Stmt
{
	consts_stmt(AsmReg *dst, const std::string &val)
		: dst(dst)
		, value(val)
	{}
	AsmString value;
	AsmReg *dst;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "consts " << *dst << " '" << value.value
			<< "'";
	}
};

struct consthash_stmt
: public Stmt
{
	consthash_stmt(AsmReg *dst, const std::string &val)
		: dst(dst)
		, value(val)
	{}
	AsmHashTag value;
	AsmReg *dst;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "consthash " << *dst << " #" << value.value;
	}
};

struct copy_stmt
: public Stmt
{
	copy_stmt(AsmReg *dst, AsmReg *src)
		: dst(dst)
		, src(src)
	{}
	AsmReg *dst;
	AsmReg *src;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "copy " << *dst << " " << *src;
	}
};

struct dfunc_stmt
: public Stmt
{
	dfunc_stmt(const std::string &fname, uint8_t arity)
		: name(fname)
		, code(NULL)
		, arity(arity)
		, func(NULL)
	{}
	AsmString name;
	Stmt::List *code;
	AsmFunc *func;
	uint8_t arity;

	bool has_code() const { return code && !code->empty(); }

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &out) const
	{
		out << "dfunc " << name.value << "/" << (uint16_t) arity;
	}
};

struct dprotocol_stmt
: public Stmt
{
	dprotocol_stmt(const std::string &protoname, uint8_t arity)
		: name(protoname)
		, arity(arity)
	{}

	AsmString name;
	Stmt::List *functions;
	AsmProtocol *protocol;
	uint8_t arity;

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &out) const
	{
		out << "dprotocol " << name.value << "/" << (uint16_t) arity;
	}
};

struct dpolymorph_stmt
: public Stmt
{
	dpolymorph_stmt(AsmModSym *protocol)
	: protocol(protocol)
	, morph_stmts(NULL)
	, functions(NULL)
	, morph_types(NULL)
	{}
	AsmModSym *protocol;
	Stmt::List *morph_stmts;
	Stmt::List *functions;
	AsmModSymList *morph_types;
	AsmPolymorph *polymorph;

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &out) const
	{
		out << "dpolymorph " << *protocol;
	}
};

struct morphtype_stmt
: public Stmt
{
	morphtype_stmt(AsmModSym *type)
		: type(type)
	{}
	AsmModSym *type;
	AsmModSym *asm_type;

	void pretty(std::ostream &out) const
	{
		out << "morphtype " << *type;
	}
	void collect_resources(ResourceSet &rs)
	{
		collect_modsym(rs, *type);
	}
};

struct fork_stmt
: public Stmt
{
	fork_stmt(AsmReg *result)
	: code(NULL)
	, dst(result)
	{}
	Stmt::List *code;
	AsmReg *dst;

	bool has_code() const { return code && !code->empty(); }

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "fork " << *dst;
	}
};

struct goto_stmt
: public Stmt
{
	goto_stmt(const std::string &lbl)
	: label(lbl)
	{}
	AsmLabel label;

	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "goto #" << label.name;
	}
};

struct label_stmt
: public Stmt
{
	label_stmt(const std::string &lbl)
	: label(lbl)
	{}
	AsmLabel label;

	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "#" << label.name;
	}
};

struct lcontext_stmt
: public Stmt
{
	lcontext_stmt(AsmReg *dst, const std::string &name)
	: dst(dst)
	, name(name)
	{}
	AsmReg *dst;
	AsmHashTag name;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &rs)
	{
		collect_resource(rs, name);
	}
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "lcontext " << *dst << " #" << name.value;
	}
};

struct lfunc_stmt
: public Stmt
{
	lfunc_stmt(AsmReg *dst, AsmModSym *ms)
	: dst(dst)
	, modsym(ms)
	{}
	AsmReg *dst;
	AsmModSym *modsym;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &rs)
	{
		collect_modsym(rs, *modsym);
	}
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "lfunc " << *dst << " " << *modsym;
	}
};

struct lpfunc_stmt
: public Stmt
{
	lpfunc_stmt(AsmReg *dst, AsmModSym *ms, const std::string &fname)
	: dst(dst)
	, protocol(ms)
	, function(fname)
	{}
	AsmReg *dst;
	AsmModSym *protocol;
	AsmString function;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &rs)
	{
		collect_modsym(rs, *protocol);
		collect_string(rs, function);
	}
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "lpfunc " << dst <<' '<< *protocol
			<<' '<< function.value;
	}
};

/** Create a new process
 *
 * Launch the new process with function in the 2nd register
 * Store the pid in the first register
 */
struct newproc_stmt
: public Stmt
{
	newproc_stmt(AsmReg *pid, AsmReg *func)
	: pid(pid)
	, func(func)
	{}
	AsmReg *pid;
	AsmReg *func;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "newproc " << *pid <<' '<< *func;
	}
};

/** Create a new process
 *
 * Launch the new process with function in the 2nd register
 * Store the pid in the first register
 */
struct recv_stmt
: public Stmt
{
	recv_stmt(AsmReg *dst, AsmReg *tube)
	: dst(dst)
	, tube(tube)
	{}
	AsmReg *dst;
	AsmReg *tube;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "recv " << *dst <<' '<< *tube;
	}
};

struct stracc_stmt
: public Stmt
{
	stracc_stmt(AsmReg *dst, AsmReg *src)
	: dst(dst)
	, src(src)
	{}
	AsmReg *dst;
	AsmReg *src;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "stracc " << *dst <<' '<< *src;
	}
};

struct unimorph_stmt
: public Stmt
{
	unimorph_stmt(AsmReg *pfreg, AsmReg *value)
	: pfreg(pfreg)
	, valreg(value)
	{}
	AsmReg *pfreg;
	AsmReg *valreg;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "morph " << *pfreg <<' '<< valreg;
	}
};

struct wait_stmt
: public Stmt
{
	wait_stmt(AsmReg *reg)
	: reg(reg)
	{}
	AsmReg *reg;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "wait " << *reg;
	}
};

struct ref_stmt
: public Stmt
{
	ref_stmt(AsmReg *dst, AsmReg *src)
		: dst(dst)
		, src(src)
	{}
	AsmReg *dst;
	AsmReg *src;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "ref " << *dst << " " << *src;
	}
};

struct return_stmt
: public Stmt
{
	return_stmt() {}

	void generate_code(AsmFunc &);
	void pretty(std::ostream &out) const
	{
		out << "return";
	}
};


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

struct AsmFunc
: public AsmResource
{
	Stmt::List *stmts;
	codeblock code;
	label_map label;
	std::list< jump_data > jump;
	AsmResource *ctx;
	const AsmString &name;
	AsmString doc;
	AsmString filename;
	uint16_t line_no;
	uint8_t ctx_type;
	uint8_t argc;
	uint8_t regc;

	AsmFunc(const AsmString &name)
		: AsmResource(RESOURCE_FUNCTION)
		, stmts(NULL)
		, ctx(NULL)
		, name(name)
		, doc()
		, line_no(0)
		, ctx_type(PFC_NULL)
		, argc(0)
		, regc(0)
	{}

	bool has_code() const;
	uint8_t fcontext() const;
	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &o) const
	{
		o << "function:" << name.value;
		return o;
	}
};

struct DeclList
{
	std::list< AsmFunc * > func;
	// std::list< AsmProtocol * > proto;
};

extern Stmt::List *parsed_stmts;

void set_function_context(Stmt::List &, uint8_t asmfc, AsmResource *);
void allocate_registers(Stmt::List &, RegAlloc *);

#endif
