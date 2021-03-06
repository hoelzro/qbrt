#ifndef QBRT_STMT_H
#define QBRT_STMT_H

#include "qbrt/resourcetype.h"
#include <string>
#include <list>
#include <map>
#include <stdint.h>

class AsmConstruct;
class AsmDataType;
class AsmFunc;
class AsmLabel;
class AsmModSym;
class AsmParam;
class AsmPolymorph;
class AsmProtocol;
class AsmReg;
class AsmResource;
class AsmString;
class AsmTypeSpec;
class RegAlloc;
typedef std::list< AsmModSym * > AsmModSymList;
typedef std::list< AsmParam * > AsmParamList;
typedef std::list< AsmString * > AsmStringList;
typedef std::list< AsmTypeSpec * > AsmTypeSpecList;
struct ResourceSet;


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

	virtual uint32_t write(std::ostream &) const;
	virtual std::ostream & pretty(std::ostream &) const;

	friend bool operator < (const AsmString &a, const AsmString &b)
	{
		return a.value < b.value;
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
	virtual void pretty(std::ostream &) const {}
};

struct dparam_stmt
: public Stmt
{
	dparam_stmt(const std::string &name, AsmTypeSpec *type)
	: name(name)
	, type(type)
	{}
	AsmString name;
	AsmTypeSpec *type;

	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;

	typedef std::list< dparam_stmt * > List;

	static void collect(AsmParamList &, std::string &param_types, List *);
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

struct bindtype_stmt
: public Stmt
{
	bindtype_stmt(AsmTypeSpec *type)
	: bindtype(type)
	{}

	AsmTypeSpec *bindtype;

	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
};

struct bind_stmt
: public Stmt
{
	bind_stmt(AsmModSym *protocol)
	: protocol(protocol)
	, params(NULL)
	, polymorph(NULL)
	{}

	AsmModSym *protocol;
	std::list< bindtype_stmt * > *params;
	Stmt::List *functions;
	AsmPolymorph *polymorph;

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
};

struct if_stmt
: public Stmt
{
	if_stmt(bool check, AsmReg *reg, const std::string &lbl)
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

struct iffail_stmt
: public Stmt
{
	iffail_stmt(bool check, AsmReg *reg, const std::string &lbl)
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
		, a(NULL)
		, b(NULL)
	{}
	call_stmt(AsmReg *result, AsmReg *func, AsmReg *p0)
	: result(result)
	, function(func)
	, a(p0)
	, b(NULL)
	{}
	call_stmt(AsmReg *result, AsmReg *func, AsmReg *p0, AsmReg *p1)
	: result(result)
	, function(func)
	, a(p0)
	, b(p1)
	{}
	AsmReg *result;
	AsmReg *function;
	AsmReg *a;
	AsmReg *b;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

struct cfailure_stmt
: public Stmt
{
	cfailure_stmt(AsmReg *dst, const std::string &type)
	: type(type)
	, dst(dst)
	{}

	AsmReg *dst;
	AsmHashTag type;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

struct cmp_stmt
: public Stmt
{
	AsmReg *result;
	AsmReg *a;
	AsmReg *b;
	int8_t opcode;

	static cmp_stmt * eq(AsmReg *result, AsmReg *, AsmReg *);
	static cmp_stmt * noteq(AsmReg *result, AsmReg *, AsmReg *);
	static cmp_stmt * gt(AsmReg *result, AsmReg *, AsmReg *);
	static cmp_stmt * gteq(AsmReg *result, AsmReg *, AsmReg *);
	static cmp_stmt * lt(AsmReg *result, AsmReg *, AsmReg *);
	static cmp_stmt * lteq(AsmReg *result, AsmReg *, AsmReg *);

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;

private:
	cmp_stmt(uint8_t op, AsmReg *result, AsmReg *a, AsmReg *b)
	: result(result)
	, a(a)
	, b(b)
	, opcode(op)
	{}
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

struct construct_stmt
: public Stmt
{
	AsmString name;
	dparam_stmt::List *fields;
	AsmConstruct *construct;

	construct_stmt(const std::string &name)
	: name(name)
	, fields(NULL)
	, construct(NULL)
	{}

	void set_function_context(uint8_t, AsmResource *);
	void collect_resources(ResourceSet &);
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

struct ctuple_stmt
: public Stmt
{
	ctuple_stmt(AsmReg *dst, uint8_t size)
	: dst(dst)
	, size(size)
	{}

	AsmReg *dst;
	uint8_t size;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

struct datatype_stmt
: public Stmt
{
	AsmString name;
	AsmStringList *args;
	Stmt::List *constructs;
	AsmDataType *datatype;

	datatype_stmt(const std::string &name, AsmStringList *args)
	: name(name)
	, args(args)
	, constructs(NULL)
	, datatype(NULL)
	{}

	uint8_t argc() const { return args ? args->size() : 0; }
	void set_function_context(uint8_t, AsmResource *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
};

struct dfunc_stmt
: public Stmt
{
	dfunc_stmt(const std::string &fname, AsmTypeSpec *result, bool abstract)
		: name(fname)
		, protocol_name(NULL)
		, result(result)
		, params(NULL)
		, code(NULL)
		, func(NULL)
		, fcontext(PFC_NULL)
		, abstract(abstract)
	{}
	AsmString name;
	AsmString *protocol_name;
	AsmTypeSpec *result;
	dparam_stmt::List *params;
	Stmt::List *code;
	AsmFunc *func;
	uint8_t fcontext;
	bool abstract;

	bool has_code() const { return code && !code->empty(); }
	uint16_t argc() const { return params ? params->size() : 0; }

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
};

struct protocol_stmt
: public Stmt
{
	protocol_stmt(const std::string &protoname
			, std::list< AsmString * > *types)
	: name(protoname)
	, typevar(types)
	, functions(NULL)
	, protocol(NULL)
	{}

	AsmString name;
	std::list< AsmString * > *typevar;
	Stmt::List *functions;
	AsmProtocol *protocol;

	void set_function_context(uint8_t, AsmResource *);
	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void pretty(std::ostream &) const;
};

struct fieldget_stmt
: public Stmt
{
	fieldget_stmt(AsmReg *dst, AsmReg *src, const std::string &name)
	: dst(dst)
	, src(src)
	, field_name(name)
	{}
	AsmReg *dst;
	AsmReg *src;
	AsmString field_name;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

struct fieldset_stmt
: public Stmt
{
	fieldset_stmt(AsmReg *dst, const std::string &name, AsmReg *src)
	: dst(dst)
	, field_name(name)
	, src(src)
	{}
	AsmReg *dst;
	AsmString field_name;
	AsmReg *src;

	void allocate_registers(RegAlloc *);
	void collect_resources(ResourceSet &);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
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

struct lconstruct_stmt
: public Stmt
{
	lconstruct_stmt(AsmReg *dst, AsmModSym *ms)
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

/**
 * Pattern match on a given register value
 */
struct match_stmt
: public Stmt
{
	match_stmt(AsmReg *result, AsmReg *patt, AsmReg *in
			, const std::string &nonmatch)
	: result(result)
	, pattern(patt)
	, input(in)
	, nonmatch(nonmatch)
	{}

	AsmReg *result;
	AsmReg *pattern;
	AsmReg *input;
	AsmLabel nonmatch;

	void allocate_registers(RegAlloc *);
	void generate_code(AsmFunc &);
	void pretty(std::ostream &) const;
};

/**
 * Pattern match on the function arguments
 */
struct matchargs_stmt
: public Stmt
{
	matchargs_stmt(AsmReg *result, AsmReg *patt
			, const std::string &nonmatch)
	: result(result)
	, pattern(patt)
	, nonmatch(nonmatch)
	{}

	AsmReg *result;
	AsmReg *pattern;
	AsmLabel nonmatch;

	void allocate_registers(RegAlloc *);
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

/** Set a register to match a pattern */
struct patternvar_stmt
: public Stmt
{
	patternvar_stmt(AsmReg *dst)
	: dst(dst)
	{}
	AsmReg *dst;

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
	recv_stmt(AsmReg *dst)
	: dst(dst)
	{}
	AsmReg *dst;

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
