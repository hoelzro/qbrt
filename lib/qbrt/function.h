#ifndef QBRT_FUNCTION_H
#define QBRT_FUNCTION_H

#include "qbrt/core.h"
#include "qbrt/type.h"
#include "qbrt/resourcetype.h"
#include <set>
#include <stack>
#include <iostream>
#include <sstream>



struct call_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t result_reg : 16;
	uint64_t func_reg : 16;
	uint64_t reserved : 24;

	call_instruction(reg_t result, reg_t func)
		: opcode_data(OP_CALL)
		, result_reg(result)
		, func_reg(func)
	{}

	static const uint8_t SIZE = 5;
};

struct return_instruction
: public instruction
{
	uint8_t opcode_data;

	return_instruction()
		: opcode_data(OP_RETURN)
	{}

	static const uint8_t SIZE = 1;
};

struct cfailure_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t hashtag_id : 16;
	uint32_t reserved : 8;

	cfailure_instruction(uint16_t hash_id)
	: opcode_data(OP_CFAILURE)
	, hashtag_id(hash_id)
	{}

	static const uint8_t SIZE = 3;
};

struct lcontext_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t hashtag : 16;
	uint64_t reserved : 24;

	lcontext_instruction(reg_t r, uint16_t hashtag)
		: opcode_data(OP_LCONTEXT)
		, reg(r)
		, hashtag(hashtag)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct lfunc_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t modsym : 16;
	uint64_t reserved : 24;

	lfunc_instruction(reg_t r, uint16_t modsym)
		: opcode_data(OP_LFUNC)
		, reg(r)
		, modsym(modsym)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct lpfunc_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t modsym : 16;
	uint64_t funcname : 16;
	uint64_t reserved : 8;

	lpfunc_instruction(reg_t r, uint16_t modsym, uint16_t func)
		: opcode_data(OP_LPFUNC)
		, reg(r)
		, modsym(modsym)
		, funcname(func)
		, reserved(0)
	{}

	static const uint8_t SIZE = 7;
};

struct loadtype_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t reg : 16;
	uint64_t modname : 16;
	uint64_t type : 16;
	uint64_t reserved : 8;

	loadtype_instruction(reg_t r, uint16_t mod, uint16_t type)
		: opcode_data(OP_LFUNC)
		, reg(r)
		, modname(mod)
		, type(type)
		, reserved(0)
	{}

	static const uint8_t SIZE = 7;
};

struct loadobj_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t modname : 16;
	uint32_t reserved : 8;

	loadobj_instruction(uint16_t mod)
		: opcode_data(OP_LOADOBJ)
		, modname(mod)
		, reserved(0)
	{}

	static const uint8_t SIZE = 3;
};

/**
 * polymorph a function with the type of a single value
 */
struct unimorph_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t funcreg : 16;
	uint64_t valreg : 16;
	uint64_t reserved : 24;

	unimorph_instruction(uint16_t funcreg, uint16_t valreg)
		: opcode_data(OP_UNIMORPH)
		, funcreg(funcreg)
		, valreg(valreg)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct move_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t dst : 16;
	uint64_t src : 16;
	uint64_t reserved : 24;

	move_instruction(reg_t dst, reg_t src)
		: opcode_data(OP_MOVE)
		, dst(dst)
		, src(src)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct ref_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t dst : 16;
	uint64_t src : 16;
	uint64_t reserved : 24;

	ref_instruction(reg_t dst, reg_t src)
		: opcode_data(OP_REF)
		, dst(dst)
		, src(src)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};

struct copy_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t dst : 16;
	uint64_t src : 16;
	uint64_t reserved : 24;

	copy_instruction(reg_t dst, reg_t src)
		: opcode_data(OP_COPY)
		, dst(dst)
		, src(src)
		, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};


struct Module;

struct PolymorphArg
{
	std::string module;
	std::string type;
	uint8_t arg_index;

	friend bool operator < (const PolymorphArg &, const PolymorphArg &);
};

struct ParamResource
{
	uint16_t name_idx;
	uint16_t type_idx;
};

struct FunctionHeader
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t file_idx;
	uint16_t line_no;
	uint16_t context_idx;
	uint16_t param_types_idx;
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

	static const uint16_t SIZE = 16;
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

	uint16_t type[];

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
	const FunctionHeader *header;
	const uint8_t *code;
	const Module *mod;

	Function(const FunctionHeader *h, const Module *m)
	: header(h)
	, code(h->code())
	, mod(m)
	{}
	Function()
	: header(NULL)
	, code(NULL)
	, mod(NULL)
	{}

	operator bool () const { return header; }
	bool operator ! () const { return ! header; }

	const char * name() const;
	inline uint8_t argc() const { return header->argc; }
	inline uint8_t regc() const { return header->regc; }
	uint32_t code_offset() const;
};

static inline uint8_t regtotal(const Function &f)
{
	return f.header->argc + f.header->regc;
}

struct function_value
: public qbrt_value_index
{
	Function func;
	qbrt_value *reg;

	function_value(const Function &f)
		: func(f)
		, reg(new qbrt_value[regtotal(f)])
	{}

	uint8_t num_values() const { return regtotal(func); }
	qbrt_value & value(uint8_t r) { return reg[r]; }
	const qbrt_value & value(uint8_t r) const { return reg[r]; }
};

void load_function_param_types(std::string &typestr, const function_value &);


struct cfunction_value
: public qbrt_value_index
{
	c_function func;
	qbrt_value *reg;
	uint8_t regc;

	cfunction_value(c_function f)
		: func(f)
		, reg(new qbrt_value[10])
		, regc(10)
	{}

	uint8_t num_values() const { return regc; }
	qbrt_value & value(uint8_t r) { return reg[r]; }
	const qbrt_value & value(uint8_t r) const { return reg[r]; }
};


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
	qbrt_value type;
	std::ostringstream debug;
	std::ostringstream usage;
	const char *function_name;
	int pc;
	int exit_code;
	int http_code;

	Failure(const std::string type_label)
	: type()
	, debug()
	, usage()
	, function_name(NULL)
	, pc(-1)
	, exit_code(-1)
	, http_code(0)
	{
		qbrt_value::hashtag(type, type_label);
	}

	Failure(const std::string type_label, const char *fname, int pc)
	: type()
	, debug()
	, usage()
	, function_name(fname)
	, pc(pc)
	, exit_code(-1)
	, http_code(0)
	{
		qbrt_value::hashtag(type, type_label);
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

	std::string debug_msg() const
	{
		std::ostringstream msg;
		if (function_name) {
			msg << function_name;
			msg << ':' << pc << ' ';
		}
		msg << debug.str();
		return msg.str();
	}
	std::string usage_msg() const { return usage.str(); }

	const std::string & typestr() const
	{
		return *type.data.str;
	}
	uint8_t num_values() const { return 1; }
	qbrt_value & value(uint8_t i)
	{
		if (i != 0) {
			std::cerr << "no Failure value at index: " << i << "\n";
		}
		return type;
	}
	const qbrt_value & value(uint8_t i) const
	{
		if (i != 0) {
			std::cerr << "no Failure value at const index: "
				<< i << "\n";
		}
		return type;
	}
};

#define NEW_FAILURE(type, fname, pc) (new Failure(type, fname, pc))
#define DUPE_FAILURE(fail, fname, pc) (new Failure(fail, fname, pc))
#define FAIL_TYPE(fname, pc) (NEW_FAILURE("typefailure", fname, pc))
#define FAIL_NOFUNCTION(fname, pc) (NEW_FAILURE("nofunction", fname, pc))

#endif
