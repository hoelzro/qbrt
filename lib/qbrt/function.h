#ifndef QBRT_FUNCTION_H
#define QBRT_FUNCTION_H

#include "qbrt/core.h"
#include "qbrt/type.h"
#include "qbrt/resourcetype.h"
#include <set>
#include <stack>
#include <iostream>
#include <sstream>


struct Module;

struct FunctionHeader
{
private:
	uint16_t _name_idx;
	uint16_t _doc_idx;
	uint16_t _line_no;
	uint16_t _context_idx;
	uint16_t _param_types_idx;
	uint16_t _result_type_idx;

public:
	uint8_t fcontext;
	uint8_t argc;
	uint8_t regc;
	uint8_t reserved;
	ParamResource params[1];

	uint16_t name_idx() const { return be16toh(_name_idx); }
	uint16_t doc_idx() const { return be16toh(_doc_idx); }
	uint16_t line_no() const { return be16toh(_line_no); }
	uint16_t context_idx() const { return be16toh(_context_idx); }
	uint16_t param_types_idx() const { return be16toh(_param_types_idx); }
	uint16_t result_type_idx() const { return be16toh(_result_type_idx); }

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
private:
	uint16_t _name_idx;
	uint16_t _doc_idx;
	uint16_t _line_no;
	uint16_t _arg_func_count;
	uint16_t _typevars[];

public:
	inline uint16_t name_idx() const { return be16toh(_name_idx); }
	inline uint16_t doc_idx() const { return be16toh(_doc_idx); }
	inline uint16_t line_no() const { return be16toh(_line_no); }
	inline uint16_t func_count() const
	{
		return be16toh(_arg_func_count) & 0x3f;
	}
	inline uint8_t argc() const
	{
		return be16toh(_arg_func_count) >> 10;
	}

	inline uint16_t typevar_idx(uint16_t i) const
	{
		return be16toh(_typevars[i]);
	}

	static const uint16_t SIZE = 8;
};

struct PolymorphResource
{
private:
	uint16_t _protocol_idx;
	uint16_t _doc_idx;
	uint16_t _line_no;
	uint16_t _type_count;
	uint16_t _func_count;

	uint16_t _type[]; // TypeSpec array

public:
	uint16_t protocol_idx() const { return be16toh(_protocol_idx); }
	uint16_t doc_idx() const { return be16toh(_doc_idx); }
	uint16_t line_no() const { return be16toh(_line_no); }
	uint16_t type_count() const { return be16toh(_type_count); }
	uint16_t func_count() const { return be16toh(_func_count); }

	uint16_t type(uint16_t i) const { return be16toh(_type[i]); }

	static const uint16_t HEADER_SIZE = 10;
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


static inline qbrt_value * follow_ref(qbrt_value *val)
{
	qbrt_value *ref = val;
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	return ref;
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

struct FailureEvent
{
	qbrt_value module;
	qbrt_value function;
	qbrt_value pc;
	qbrt_value c_file;
	qbrt_value c_lineno;

	FailureEvent()
	: module(TYPE_STRING)
	, function(TYPE_STRING)
	, pc(TYPE_INT)
	, c_file(TYPE_STRING)
	, c_lineno(TYPE_INT)
	, direction(0)
	{}

	void up(const std::string &mod, const std::string &func, uint16_t pc)
	{
		qbrt_value::str(module, mod);
		qbrt_value::str(function, func);
		qbrt_value::i(this->pc, pc);
		direction = +1;
	}
	void down(const std::string &mod, const std::string &func, uint16_t pc
			, const char *cfile, int cline)
	{
		set(mod, func, pc, cfile, cline);
		direction = -1;
	}
	void set(const std::string &mod, const std::string &func, uint16_t pc
			, const char *cfile, int cline)
	{
		qbrt_value::str(module, mod);
		qbrt_value::str(function, func);
		qbrt_value::i(this->pc, pc);
		qbrt_value::str(c_file, cfile);
		qbrt_value::i(c_lineno, cline);
	}

	friend std::ostream & operator << (std::ostream &,const FailureEvent &);

private:
	int direction;
};

struct Failure
: public qbrt_value_index
{
	qbrt_value type;		// 0
	qbrt_value exit_code;		// 1
	int http_code;			// 2
	std::ostringstream debug;	// 3
	std::ostringstream usage;	// 4
	// these will go to the call stack
	std::list< FailureEvent > trace;

	Failure(const std::string type_label, const std::string &module
			, const char *fname, int pc
			, const char *cfile, int cline);

	std::string debug_msg() const;
	std::string usage_msg() const { return usage.str(); }

	const std::string & typestr() const
	{
		return *type.data.str;
	}
	uint8_t num_values() const { return 1; }
	qbrt_value & value(uint8_t);
	const qbrt_value & value(uint8_t) const;

	void trace_up(const std::string &mod, const std::string &fname
			, uint16_t pc);
	void trace_down(const std::string &mod, const std::string &fname
			, uint16_t pc, const char *c_file, int c_line);

	static void write(std::ostream &, const Failure &);
	static void write_trace(std::ostream &
			, const std::list< FailureEvent > &);
};

#define NEW_FAILURE(type, mod, fname, pc) \
		(new Failure(type, mod, fname, pc, __FILE__, __LINE__))
#define FAIL_TYPE(mod, fname, pc) (NEW_FAILURE("typefailure", mod, fname, pc))
#define FAIL_MODULE404(mod, fname, pc) \
		(NEW_FAILURE("module404", mod, fname, pc))
#define FAIL_FUNCTION404(mod, fname, pc) \
		(NEW_FAILURE("function404", mod, fname, pc))
#define FAIL_REGISTER404(mod, fname, pc) \
		(NEW_FAILURE("register404", mod, fname, pc))

#endif
