#ifndef QBRT_FUNCTION_H
#define QBRT_FUNCTION_H

#include "qbrt/core.h"
#include "qbrt/type.h"
#include "qbrt/resourcetype.h"
#include <set>
#include <stack>
#include <iostream>
#include <sstream>


#pragma pack(push, 1)

struct call_instruction
: public instruction
{
	uint16_t result_reg;
	uint16_t func_reg;

	call_instruction(reg_t result, reg_t func)
		: instruction(OP_CALL)
		, result_reg(result)
		, func_reg(func)
	{}

	static const uint8_t SIZE = 5;
};

struct return_instruction
: public instruction
{
	return_instruction()
	: instruction(OP_RETURN)
	{}

	static const uint8_t SIZE = 1;
};

struct cfailure_instruction
: public instruction
{
	uint16_t dst;
	uint16_t hashtag_id;

	cfailure_instruction(reg_t dst, uint16_t hash_id)
	: instruction(OP_CFAILURE)
	, dst(dst)
	, hashtag_id(hash_id)
	{}

	static const uint8_t SIZE = 5;
};

struct lcontext_instruction
: public instruction
{
	uint16_t reg;
	uint16_t hashtag;

	lcontext_instruction(reg_t r, uint16_t hashtag)
		: instruction(OP_LCONTEXT)
		, reg(r)
		, hashtag(hashtag)
	{}

	static const uint8_t SIZE = 5;
};

struct lfunc_instruction
: public instruction
{
	uint16_t reg;
	uint16_t modsym;

	lfunc_instruction(reg_t r, uint16_t modsym)
		: instruction(OP_LFUNC)
		, reg(r)
		, modsym(modsym)
	{}

	static const uint8_t SIZE = 5;
};

struct loadtype_instruction
: public instruction
{
	uint16_t reg;
	uint16_t modname;
	uint16_t type;

	loadtype_instruction(reg_t r, uint16_t mod, uint16_t type)
		: instruction(OP_LFUNC)
		, reg(r)
		, modname(mod)
		, type(type)
	{}

	static const uint8_t SIZE = 7;
};

struct loadobj_instruction
: public instruction
{
	uint16_t modname;

	loadobj_instruction(uint16_t mod)
		: instruction(OP_LOADOBJ)
		, modname(mod)
	{}

	static const uint8_t SIZE = 3;
};

struct move_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	move_instruction(reg_t dst, reg_t src)
		: instruction(OP_MOVE)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

struct ref_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	ref_instruction(reg_t dst, reg_t src)
		: instruction(OP_REF)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

struct copy_instruction
: public instruction
{
	uint16_t dst;
	uint16_t src;

	copy_instruction(reg_t dst, reg_t src)
		: instruction(OP_COPY)
		, dst(dst)
		, src(src)
	{}

	static const uint8_t SIZE = 5;
};

#pragma pack(pop)


struct Module;

struct PolymorphArg
{
	std::string module;
	std::string type;
	uint8_t arg_index;

	friend bool operator < (const PolymorphArg &, const PolymorphArg &);
};

struct FunctionHeader
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t file_idx;
	uint16_t line_no;
	uint16_t context_idx;
	uint16_t param_types_idx;
	uint16_t result_type_idx;
	uint8_t fcontext;
	uint8_t argc;
	uint8_t regc;
	uint8_t reserved;
	ParamResource params[];

	/** Get the address for where code starts */
	const uint8_t * code() const
	{
		return ((const uint8_t *) this)
			+ SIZE + argc * sizeof(ParamResource);
	}

	static const uint16_t SIZE = 18;
};

struct ProtocolResource
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t file_idx;
	uint16_t line_no;
	uint16_t arg_func_count;

	inline uint16_t argc() const
	{ return endian_swap(arg_func_count) >> 10; }

	inline uint16_t func_count() const
	{ return endian_swap(arg_func_count) & 0x03ff; }

	inline uint16_t typevar_idx(uint16_t i) const
	{
		return (&arg_func_count)[i+1];
	}

	static const uint16_t SIZE = 10;
};

struct PolymorphResource
{
	uint16_t protocol_idx;
	uint16_t doc_idx;
	uint16_t file_idx;
	uint16_t line_no;
	uint16_t type_count;
	uint16_t func_count;

	uint16_t type[]; // TypeSpec array

	static const uint16_t HEADER_SIZE = 12;
};

struct ContextStack
{
	std::map< std::string, std::stack< qbrt_value > > value;

	qbrt_value & push(const std::string &name)
	{
		std::stack< qbrt_value > &valstack(value[name]);
		valstack.push(qbrt_value());
		return valstack.top();
	}

	void pop(const std::string &name)
	{
		value[name].pop();
	}

	qbrt_value * top(const std::string &name)
	{
		std::map< std::string, std::stack< qbrt_value > >
			::iterator it;
		it = value.find(name);
		if (it == value.end()) {
			return NULL;
		}
		return &it->second.top();
	}

	const qbrt_value * top(const std::string &name) const
	{
		std::map< std::string, std::stack< qbrt_value > >
			::const_iterator it;
		it = value.find(name);
		if (it == value.end()) {
			return NULL;
		}
		return &it->second.top();
	}
};

struct Function
{
	const Module *mod;

	Function(const Module *m)
	: mod(m)
	{}

	virtual const char * name() const = 0;
	virtual uint8_t argc() const  = 0;
	virtual uint8_t regc() const = 0;
	virtual uint8_t fcontext() const = 0;
	virtual c_function cfunc() const = 0;
	virtual const char * protocol_module() const = 0;
	virtual const char * protocol_name() const = 0;

	inline uint8_t regtotal() const { return argc() + regc(); }
};

struct QbrtFunction
: public Function
{
	const FunctionHeader *header;
	const uint8_t *code;

	QbrtFunction(const FunctionHeader *h, const Module *m)
	: Function(m)
	, header(h)
	, code(h->code())
	{}

	const char * name() const;
	uint8_t argc() const { return header->argc; }
	uint8_t regc() const { return header->regc; }
	uint8_t fcontext() const { return header->fcontext; }
	c_function cfunc() const { return NULL; }
	const char * protocol_module() const;
	const char * protocol_name() const;

	uint32_t code_offset() const;
};

struct CFunction
: public Function
{
	c_function function;
	const std::string param_types;
	std::string proto_module;
	std::string proto_name;

	CFunction(c_function f, const Module *mod, const std::string &name
			, uint8_t argc, const std::string &paramtypes)
	: Function(mod)
	, function(f)
	, _name(name)
	, param_types(paramtypes)
	, _argc(argc)
	, fctx(PFC_NONE)
	{}

	void set_protocol(const std::string &mod, const std::string &name)
	{
		proto_module = mod;
		proto_name = name;
		fctx = PFC_OVERRIDE;
	}

	const char * name() const { return _name.c_str(); }
	uint8_t argc() const { return _argc; }
	uint8_t regc() const { return _argc; }
	uint8_t fcontext() const { return fctx; }
	c_function cfunc() const { return function; }
	const char * protocol_module() const;
	const char * protocol_name() const;

private:
	const std::string _name;
	const uint8_t _argc;
	uint8_t fctx;
};

struct function_value
: public qbrt_value_index
{
	const Function *func;
	qbrt_value *regv;
	uint8_t argc;
	uint8_t regc;

	function_value(const Function *);
	void realloc(uint8_t regc);

	uint8_t fcontext() const { return func->fcontext(); }
	bool traditional() const
	{
		return fcontext() == PFC_NONE;
	}
	bool abstract() const
	{
		return fcontext() == PFC_ABSTRACT;
	}
	const char * name() const { return func->name(); }

	uint8_t num_values() const { return regc; }
	qbrt_value & value(uint8_t r) { return regv[r]; }
	const qbrt_value & value(uint8_t r) const { return regv[r]; }
};

void load_function_value_types(std::ostringstream &, const function_value &);
void reassign_func(function_value &funcval, const Function *newfunc);


static inline qbrt_value & follow_ref(qbrt_value &val)
{
	qbrt_value *ref = &val;
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	return *ref;
}


#define QBRT_FUNCTION_INFO_SIZE 10

static inline const char * fcontext_name(int fc) {
	switch (fc) {
		case PFC_NONE:
			return "traditional";
		case PFC_ABSTRACT:
			return "abstract protocol";
		case PFC_DEFAULT:
			return "default protocol";
		case PFC_OVERRIDE:
			return "polymorph";
		case PFC_NULL:
			return "null_fcontext";
	}
	return "wtf_is_that_fcontext";
}

struct Failure
: public qbrt_value_index
{
	qbrt_value type;		// 0
	qbrt_value exit_code;		// 1
	int http_code;			// 2
	std::ostringstream debug;	// 3
	std::ostringstream usage;	// 4
	// these will go to the call stack
	const char *function_name;
	int pc;

	Failure(const std::string type_label)
	: type()
	, debug()
	, usage()
	, function_name(NULL)
	, pc(-1)
	, http_code(0)
	{
		qbrt_value::hashtag(type, type_label);
		qbrt_value::i(exit_code, -1);
	}

	Failure(const std::string type_label, const char *fname, int pc)
	: type()
	, debug()
	, usage()
	, function_name(fname)
	, pc(pc)
	, http_code(0)
	{
		qbrt_value::hashtag(type, type_label);
		qbrt_value::i(exit_code, -1);
	}

	Failure(const Failure &fail, const char *fname, int pc)
	: type(fail.type)
	, debug(fail.debug.str())
	, usage(fail.usage.str())
	, function_name(fname)
	, pc(pc)
	, exit_code(fail.exit_code)
	, http_code(fail.http_code)
	{}

	std::string debug_msg() const;
	std::string usage_msg() const { return usage.str(); }

	const std::string & typestr() const
	{
		return *type.data.str;
	}
	uint8_t num_values() const { return 1; }
	qbrt_value & value(uint8_t);
	const qbrt_value & value(uint8_t) const;
};

#define NEW_FAILURE(type, fname, pc) (new Failure(type, fname, pc))
#define DUPE_FAILURE(fail, fname, pc) (new Failure(fail, fname, pc))
#define FAIL_TYPE(fname, pc) (NEW_FAILURE("typefailure", fname, pc))
#define FAIL_MODULE404(fname, pc) (NEW_FAILURE("module404", fname, pc))
#define FAIL_FUNCTION404(fname, pc) (NEW_FAILURE("function404", fname, pc))
#define FAIL_REGISTER404(fname, pc) (NEW_FAILURE("register404", fname, pc))

#endif
