#ifndef QBRT_FUNCTION_H
#define QBRT_FUNCTION_H

#include "qbrt/core.h"
#include "qbrt/type.h"
#include <set>
#include <stack>
#include <iostream>
#include <sstream>


#define PFC_NULL	0b000
#define PFC_NONE	0b001
#define PFC_ABSTRACT	0b010
#define PFC_DEFAULT	0b011
#define PFC_OVERRIDE	0b111

#define PFC_MASK_HAS_CODE 0b001
#define PFC_MASK_PROTOCOL 0b010
#define PFC_MASK_OVERRIDE 0b100

// Function Context Types (does not include code bit)
#define FCT_TRADITIONAL	0b000
#define FCT_PROTOCOL	0b010
#define FCT_POLYMORPH	0b110

#define PFC_TYPE(pfc) (pfc & 0b110)
#define FCT_WITH_CODE(fct) (fct | 0b001)


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
struct Object;

struct PolymorphArg
{
	std::string module;
	std::string type;
	uint8_t arg_index;

	friend bool operator < (const PolymorphArg &, const PolymorphArg &);
};


struct FunctionResource
{
	uint16_t name_idx;
	uint16_t doc_idx;
	uint16_t file_idx;
	uint16_t line_no;
	uint16_t context_idx;
	uint8_t fcontext;
	uint8_t argc;
	uint8_t regc;
	uint8_t code[];

	FunctionResource(uint16_t name, uint16_t doc, uint16_t file
			, uint16_t line, uint8_t argc, uint8_t regc)
		: name_idx(name)
		, doc_idx(doc)
		, file_idx(file)
		, line_no(line)
		, fcontext(PFC_NONE)
		, argc(argc)
		, regc(regc)
	{}

	static const uint16_t HEADER_SIZE = 13;
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
	const FunctionResource *resource;
	const Object *obj;
	const Module *mod;

	Function(const FunctionResource *f, const Object *o, const Module *m)
		: resource(f)
		, obj(o)
		, mod(m)
	{}
	Function()
		: resource(NULL)
		, obj(NULL)
		, mod(NULL)
	{}

	operator bool () const { return resource; }
	bool operator ! () const { return ! resource; }

	const char * name() const;
	inline uint8_t argc() const { return resource->argc; }
	inline uint8_t regc() const { return resource->regc; }
	uint32_t code_offset() const;
};

static inline uint8_t regtotal(const Function &f)
{
	return f.resource->argc + f.resource->regc;
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

struct cfunction_value
: public qbrt_value_index
{
	c_function func;
	qbrt_value *reg;

	cfunction_value(c_function f)
		: func(f)
		, reg(new qbrt_value[num_values()])
	{}

	uint8_t num_values() const { return 10; }
	qbrt_value & value(uint8_t r) { return reg[r]; }
	const qbrt_value & value(uint8_t r) const { return reg[r]; }
};


struct CContext
{
	virtual ~CContext() {}
	virtual uint8_t regc() const = 0;
	virtual qbrt_value & value(uint8_t) = 0;
	virtual qbrt_value & value(uint8_t, uint8_t) = 0;
	virtual void io(StreamIO *) = 0;
};

static inline qbrt_value & follow_ref(qbrt_value &val)
{
	qbrt_value *ref = &val;
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	return *ref;
}

class WorkerCContext
: public CContext
{
public:
	WorkerCContext(qbrt_value_index &idx, StreamIO *&io)
		: index(idx)
		, nextio(io)
	{}

	virtual uint8_t regc() const
	{
		return index.num_values();
	}
	virtual qbrt_value & value(uint8_t reg)
	{
		return follow_ref(index.value(reg));
	}
	virtual qbrt_value & value(uint8_t primary, uint8_t secondary)
	{
		qbrt_value_index *idx;
		idx = follow_ref(index.value(primary)).data.reg;
		return follow_ref(idx->value(secondary));
	}
	virtual void io(StreamIO *op)
	{
		if (nextio) {
			std::cerr << "nextio is not null!\n";
		}
		nextio = op;
	}

private:
	qbrt_value_index &index;
	StreamIO *&nextio;
};

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
#define NEW_TYPE_FAILURE(fname, pc) (NEW_FAILURE("typefailure", fname, pc))

#endif
