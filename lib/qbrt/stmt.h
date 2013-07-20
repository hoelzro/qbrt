#ifndef QBRT_STMT_H
#define QBRT_STMT_H

#include "qbrt/resourcetype.h"
#include <string>
#include <list>
#include <map>
#include <stdint.h>

class AsmFunc;
class AsmLabel;
class AsmModSym;
class AsmPolymorph;
class AsmProtocol;
class AsmReg;
class AsmResource;
class AsmString;
class RegAlloc;
typedef std::list< AsmModSym * > AsmModSymList;


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

struct ResourceLess
{
	bool operator () (const AsmResource *, const AsmResource *) const;
};

typedef std::map< AsmResource *, uint16_t, ResourceLess > ResourceSet;
static inline ResourceSet::value_type asm_resource_pair(AsmResource *r)
{
	return ResourceSet::value_type(r, 0);
}


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

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &) const;
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

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &) const;
};

struct AsmLabel
{
	std::string name;

	AsmLabel(const std::string &lbl)
	: name(lbl)
	{} 
};


struct Stmt
{
	typedef std::list< Stmt * > List;

	virtual ~Stmt() {}

	virtual void set_function_context(uint8_t, AsmResource *) {}
	virtual void allocate_registers(RegAlloc *) {}
	virtual void collect_resources(ResourceSet &) {}
	virtual void generate_code(AsmFunc &);
	virtual void pretty(std::ostream &) const = 0;
};


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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;

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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
};

struct dparam_stmt
: public Stmt
{
	dparam_stmt(const std::string &name, AsmModSym *type)
	: name(name)
	, typname(type)
	{}
	AsmString name;
	AsmModSym *typname;

	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;

	typedef std::list< dparam_stmt * > List;
};

struct dfunc_stmt
: public Stmt
{
	dfunc_stmt(const std::string &fname, uint8_t arity)
		: name(fname)
		, params(NULL)
		, code(NULL)
		, arity(arity)
		, func(NULL)
	{}
	AsmString name;
	dparam_stmt::List *params;
	Stmt::List *code;
	AsmFunc *func;
	uint8_t arity;

	bool has_code() const { return code && !code->empty(); }

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
};

struct morphtype_stmt
: public Stmt
{
	morphtype_stmt(AsmModSym *type)
		: type(type)
	{}
	AsmModSym *type;
	AsmModSym *asm_type;

	void pretty(std::ostream &) const;
	void collect_resources(ResourceSet &);
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
	void pretty(std::ostream &) const;
};

struct goto_stmt
: public Stmt
{
	goto_stmt(const std::string &lbl)
	: label(lbl)
	{}
	AsmLabel label;

	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

struct label_stmt
: public Stmt
{
	label_stmt(const std::string &lbl)
	: label(lbl)
	{}
	AsmLabel label;

	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
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
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
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
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
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
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &out) const;
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
	void pretty(std::ostream &) const;
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
	void pretty(std::ostream &) const;
};

struct return_stmt
: public Stmt
{
	return_stmt() {}

	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};


extern Stmt::List *parsed_stmts;

#endif
