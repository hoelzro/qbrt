
#include "qbrt/core.h"
#include "qbrt/schedule.h"
#include "qbrt/function.h"
#include "qbrt/tuple.h"
#include "qbrt/map.h"
#include "qbrt/vector.h"
#include "qbrt/module.h"
#include "io.h"
#include "instruction/arithmetic.h"
#include "instruction/function.h"
#include "instruction/logic.h"
#include "instruction/schedule.h"
#include "instruction/string.h"
#include "instruction/type.h"

#include <vector>
#include <stack>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;


qbrt_value CONST_REGISTER[CONST_REG_COUNT];

ostream & inspect(ostream &, const qbrt_value &);
ostream & inspect_function(ostream &, const Function &);
ostream & inspect_call_frame(ostream &, const CodeFrame &);
int inspect_indent(0);

#define RETURN_FAILURE(ctx, reg) do { \
	Failure *fail = ctx.failure(reg); \
	if (fail) { \
		fail->trace_down(ctx.module_name(), ctx.function_name(), \
			ctx.pc(), __FILE__, __LINE__); \
		FunctionCall &call(ctx.worker().current->function_call()); \
		qbrt_value::fail(*call.result, fail); \
		call.cfstate = CFS_FAILED; \
		return; \
	}} while (0)

#define READ_REG(val, ctx, reg) do { \
	val = read_reg(ctx, reg, __FILE__, __LINE__); \
	if (!val) { return; }} while (0)

#define WRITE_REG(val, ctx, reg) do { \
	val = write_reg(ctx, reg, __FILE__, __LINE__); \
	if (!val) { return; }} while (0)

#define RW_REG(val, ctx, reg) do { \
	val = rwreg(ctx, reg, __FILE__, __LINE__); \
	if (!val) { return; }} while (0)

#define READ_FAILED_REG(val, ctx, reg) do { \
	val = read_failed_reg(ctx, reg, __FILE__, __LINE__); \
	if (!val) { return; }} while (0)


void init_const_registers()
{
	for (int i(0); i<16; ++i) {
		qbrt_value::i(CONST_REGISTER[REG_CINT(i)], i);
	}
	qbrt_value::b(CONST_REGISTER[REG_FALSE], false);
	qbrt_value::b(CONST_REGISTER[REG_TRUE], true);
	qbrt_value::fp(CONST_REGISTER[REG_FZERO], 0.0);
	qbrt_value::str(CONST_REGISTER[REG_EMPTYSTR], "");
	qbrt_value::str(CONST_REGISTER[REG_NEWLINE], "\n");
	qbrt_value::list(CONST_REGISTER[REG_EMPTYLIST], NULL);
	qbrt_value::vect(CONST_REGISTER[REG_EMPTYVECT], NULL);
	CONST_REGISTER[REG_VOID] = qbrt_value();
}

qbrt_value & get_special_reg(CodeFrame &f, reg_t reg)
{
	qbrt_value *special = 0;
	switch (reg) {
		case REG_RESULT:
			special = f.function_call().result;
			break;
		default:
			cout << "dunno that special register";
			break;
	}
	return *special;
}

struct OpContext
{
	virtual uint8_t argc() const = 0;
	virtual uint8_t regc() const = 0;

	virtual qbrt_value * value(uint8_t) = 0;
	virtual qbrt_value * result() = 0;

	virtual const qbrt_value * srcvalue(uint16_t) const = 0;
	virtual qbrt_value * dstvalue(uint16_t) = 0;
	virtual qbrt_value & refvalue(uint16_t) = 0;
	virtual Failure * failure(uint16_t) = 0;

	virtual const std::string & module_name() const = 0;
	virtual const char * function_name() const = 0;
	virtual int & pc() const = 0;
	virtual Worker & worker() const = 0;
	virtual const ResourceTable & resource() const = 0;
	virtual qbrt_value * get_context(const std::string &) = 0;
	virtual void io(StreamIO *op) = 0;

	void backtrace(Failure &f)
	{
		CodeFrame::backtrace(f, worker().current->parent);
	}
	Failure * fail_frame(const char *type, const char *file, uint16_t line)
	{
		Failure *f = new Failure(type, module_name(), function_name()
				, pc(), file, line);
		qbrt_value::fail(*dstvalue(SPECIAL_REG_RESULT), f);
		worker().current->cfstate = CFS_FAILED;
	}
	void fail_frame(Failure *f)
	{
		qbrt_value::fail(*dstvalue(SPECIAL_REG_RESULT), f);
		worker().current->cfstate = CFS_FAILED;
	}
};

/**
 * Given a value, follow it's references. Then check for failure.
 */
static inline qbrt_value * readable_value(qbrt_value *val, OpContext &ctx
		, const char *file, uint16_t lineno)
{
	qbrt_value *ref(val);
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	if (ref->type->id == VT_FAILURE) {
		Failure *fail = ref->data.failure;
		fail->trace_down(ctx.module_name(), ctx.function_name(),
			ctx.pc(), file, lineno);
		FunctionCall &call(ctx.worker().current->function_call());
		qbrt_value::fail(*call.result, fail);
		call.cfstate = CFS_FAILED;
		return NULL;
	} else if (ref->type->id == VT_PROMISE) {
		Worker &w(ctx.worker());
		w.current->cfstate = CFS_PEERWAIT;
		ref->data.promise->mark_to_notify(
				w.current->waiting_for_promise);
		return NULL;
	}
	return ref;
}

static inline qbrt_value * writable_value(qbrt_value *val, OpContext &ctx
		, const char *file, uint16_t lineno)
{
	qbrt_value *ref(val);
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	// overwrite failures or promises
	return ref;
}

static inline qbrt_value * readable_failed_value(qbrt_value *val, OpContext &ctx
		, const char *file, uint16_t lineno)
{
	qbrt_value *ref(val);
	while (ref->type->id == VT_REF) {
		ref = ref->data.ref;
	}
	// don't check for failures b/c we want to hear about those in this case
	if (ref->type->id == VT_PROMISE) {
		Worker &w(ctx.worker());
		w.current->cfstate = CFS_PEERWAIT;
		ref->data.promise->mark_to_notify(
				w.current->waiting_for_promise);
		return NULL;
	}
	return ref;
}

static inline qbrt_value * primary_reg(OpContext &ctx
		, uint8_t primary, const char *file, uint16_t lineno)
{
	if (primary >= ctx.regc()) {
		ctx.fail_frame("#register404", file, lineno);
		return NULL;
	}
	return ctx.value(primary);
}

static inline qbrt_value * secondary_reg(OpContext &ctx
		, uint8_t primary, uint8_t secondary
		, const char *file, uint16_t lineno)
{
	if (primary >= ctx.regc()) {
		ctx.fail_frame("#register404", file, lineno);
		return NULL;
	}
	qbrt_value *prim(readable_value(ctx.value(primary), ctx
				, file, lineno));
	if (!prim) {
		return NULL;
	}

	qbrt_value_index *idx(prim->data.reg);
	if (secondary >= idx->num_values()) {
		ctx.fail_frame("#register404", file, lineno);
		return NULL;
	}
	return &idx->value(secondary);
}

/**
 * Get the value stored in a given register.
 *
 * If it's a failure, copy the failure the context result and return NULL
 * If it's a promise, put the frame into wait and return NULL
 * If it's a ref, follow the ref it's actual value and return it
 * If it's a regular value, return it
 */
static inline const qbrt_value * read_reg(OpContext &ctx, uint16_t reg
		, const char *file, uint16_t lineno)
{
	uint8_t primary, secondary;
	qbrt_value *value(NULL);
	if (REG_IS_PRIMARY(reg)) {
		primary = REG_EXTRACT_PRIMARY(reg);
		value = readable_value(primary_reg(ctx, primary, file, lineno)
				, ctx, file, lineno);
	} else if (REG_IS_SECONDARY(reg)) {
		primary = REG_EXTRACT_SECONDARY1(reg);
		secondary = REG_EXTRACT_SECONDARY2(reg);
		value = secondary_reg(ctx, primary, secondary, file, lineno);
		value = readable_value(value, ctx, file, lineno);
	} else if (SPECIAL_REG_RESULT == reg) {
		// this should check for null result
		value = readable_value(ctx.result(), ctx, file, lineno);
	} else if (REG_IS_CONST(reg)) {
		value = &CONST_REGISTER[REG_EXTRACT_CONST(reg)];
	} else {
		ctx.fail_frame(FAIL_REGISTER404(ctx.module_name()
					, ctx.function_name(), ctx.pc()));
	}
	return value;
}

static inline qbrt_value * write_reg(OpContext &ctx, uint16_t reg
		, const char *file, uint16_t lineno)
{
	uint8_t primary, secondary;
	qbrt_value *value(NULL);
	if (REG_IS_PRIMARY(reg)) {
		primary = REG_EXTRACT_PRIMARY(reg);
		value = writable_value(primary_reg(ctx, primary, file, lineno)
				, ctx, file, lineno);
	} else if (REG_IS_SECONDARY(reg)) {
		primary = REG_EXTRACT_SECONDARY1(reg);
		secondary = REG_EXTRACT_SECONDARY2(reg);
		value = secondary_reg(ctx, primary, secondary, file, lineno);
		value = writable_value(value, ctx, file, lineno);
	} else if (SPECIAL_REG_RESULT == reg) {
		// this should check for null result
		value = writable_value(ctx.result(), ctx, file, lineno);
	} else if (CONST_REG_VOID == reg) {
		return &ctx.worker().drain;
	} else {
		ctx.fail_frame(FAIL_REGISTER404(ctx.module_name()
					, ctx.function_name(), ctx.pc()));
	}
	return value;
}

static inline const qbrt_value * read_failed_reg(OpContext &ctx, uint16_t reg
		, const char *file, uint16_t lineno)
{
	uint8_t primary, secondary;
	qbrt_value *value(NULL);
	if (REG_IS_PRIMARY(reg)) {
		primary = REG_EXTRACT_PRIMARY(reg);
		value = readable_failed_value(primary_reg(ctx, primary
				, file, lineno), ctx, file, lineno);
	} else if (REG_IS_SECONDARY(reg)) {
		primary = REG_EXTRACT_SECONDARY1(reg);
		secondary = REG_EXTRACT_SECONDARY2(reg);
		value = secondary_reg(ctx, primary, secondary, file, lineno);
		value = readable_failed_value(value, ctx, file, lineno);
	} else if (SPECIAL_REG_RESULT == reg) {
		// this should check for null result
		value = readable_failed_value(ctx.result(), ctx, file, lineno);
	} else {
		ctx.fail_frame(FAIL_REGISTER404(ctx.module_name()
					, ctx.function_name(), ctx.pc()));
	}
	return value;
}

class WorkerOpContext
: public OpContext
{
public:
	WorkerOpContext(Worker &w)
	: w(w)
	, frame(*w.current)
	, func(w.current->function_call())
	{}

	virtual Worker & worker() const { return w; }
	virtual const std::string & module_name() const
	{
		return func.mod->name;
	}
	virtual const char * function_name() const { return func.name(); }
	virtual int & pc() const { return frame.pc; }
	virtual uint8_t argc() const { return func.header->argc; }
	virtual uint8_t regc() const { return func.num_values(); }
	virtual qbrt_value * value(uint8_t reg)
	{
		return &func.value(reg);
	}
	virtual qbrt_value * result()
	{
		return func.result;
	}

	virtual const qbrt_value * srcvalue(uint16_t reg) const
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			if (primary >= regc()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			return follow_ref(&func.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			if (primary >= regc()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			if (qbrt_value::failed(func.value(primary))) {
				qbrt_value::fail(*func.result
					, func.value(primary).data.failure);
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			secondary = REG_EXTRACT_SECONDARY2(reg);
			qbrt_value_index *idx(func.value(primary).data.reg);
			if (secondary >= idx->num_values()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			return follow_ref(&idx->value(secondary));
		} else if (SPECIAL_REG_RESULT == reg) {
			return func.result;
		} else if (REG_IS_CONST(reg)) {
			return &CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf src reg? " << reg << endl;
		frame.cfstate = CFS_FAILED;
		return NULL;
	}

	virtual qbrt_value * dstvalue(uint16_t reg)
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			if (primary >= regc()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			return follow_ref(&func.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			if (primary >= regc()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			if (qbrt_value::failed(func.value(primary))) {
				qbrt_value::fail(*func.result
					, func.value(primary).data.failure);
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			secondary = REG_EXTRACT_SECONDARY2(reg);
			qbrt_value_index *idx(func.value(primary).data.reg);
			if (secondary >= idx->num_values()) {
				qbrt_value::fail(*func.result
					, FAIL_REGISTER404(module_name(),
						function_name(), pc()));
				frame.cfstate = CFS_FAILED;
				return NULL;
			}
			return follow_ref(&idx->value(secondary));
		} else if (CONST_REG_VOID == reg) {
			return &w.drain;
		} else if (SPECIAL_REG_RESULT == reg) {
			return func.result;
		} else if (REG_IS_CONST(reg)) {
			return &CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf dst reg? " << reg << endl;
		frame.cfstate = CFS_FAILED;
		return NULL;
	}

	virtual qbrt_value & refvalue(uint16_t reg)
	{
		if (REG_IS_PRIMARY(reg)) {
			return func.value(REG_EXTRACT_PRIMARY(reg));
		} else if (REG_IS_SECONDARY(reg)) {
			uint16_t r1(REG_EXTRACT_SECONDARY1(reg));
			uint16_t r2(REG_EXTRACT_SECONDARY2(reg));
			if (!qbrt_value::is_value_index(func.value(r1))) {
				cerr << "cannot access secondary register: "
					<< r1 << endl;
				return *(qbrt_value *) NULL;
			}
			return func.value(r1).data.reg->value(r2);
		}
		cerr << "Unsupported ref register: " << reg << endl;
		return *(qbrt_value *) NULL;
	}
	Failure * failure(uint16_t reg)
	{
		if (REG_IS_PRIMARY(reg)) {
			uint16_t r(REG_EXTRACT_PRIMARY(reg));
			if (qbrt_value::failed(func.value(r))) {
				return func.value(r).data.failure;
			}
		} else if (REG_IS_SECONDARY(reg)) {
			uint16_t r1(REG_EXTRACT_SECONDARY1(reg));
			uint16_t r2(REG_EXTRACT_SECONDARY2(reg));
			if (qbrt_value::failed(func.value(r1))) {
				return func.value(r1).data.failure;
			}
		}
		return NULL;
	}

	qbrt_value * get_context(const std::string &name)
	{
		return add_context(&frame, name);
	}

	const ResourceTable & resource() const
	{
		return func.mod->resource;
	}

	virtual void io(StreamIO *op)
	{
		frame.io_push(op);
	}

private:
	Worker &w;
	CodeFrame &frame;
	FunctionCall &func;
};

class WorkerCContext
: public OpContext
{
public:
	WorkerCContext(Worker &w, function_value &cf)
	: w(w)
	, frame(*w.current)
	, cfunc(cf)
	{}

	virtual Worker & worker() const { return w; }
	virtual const std::string & module_name() const
	{
		return cfunc.func->mod->name;
	}
	virtual const char * function_name() const { return "cfunction"; }
	virtual int & pc() const { return *(int *) NULL; }
	virtual uint8_t argc() const { return cfunc.argc; }
	virtual uint8_t regc() const { return cfunc.regc; }
	virtual qbrt_value * value(uint8_t reg)
	{
		return &cfunc.value(reg);
	}
	virtual qbrt_value * result()
	{
		cerr << "No result register available in C\n";
		exit(-1);
		return NULL;
	}

	virtual const qbrt_value * srcvalue(uint16_t reg) const
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(&cfunc.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(&(cfunc.value(primary).data.reg
				->value(secondary)));
		} else if (SPECIAL_REG_RESULT == reg) {
			cerr << "no src result register for c functions\n";
			return NULL;
		} else if (REG_IS_CONST(reg)) {
			return &CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf src reg in C? " << reg << endl;
		return NULL;
	}

	virtual qbrt_value * dstvalue(uint16_t reg)
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(&cfunc.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(&(cfunc.value(primary).data.reg
				->value(secondary)));
		} else if (CONST_REG_VOID == reg) {
			return &w.drain;
		} else if (SPECIAL_REG_RESULT == reg) {
			cerr << "no dst result register for c functions\n";
			return NULL;
		} else if (REG_IS_CONST(reg)) {
			return &CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf dst reg? " << reg << endl;
		return NULL;
	}

	virtual qbrt_value & refvalue(uint16_t reg)
	{
		if (REG_IS_PRIMARY(reg)) {
			return cfunc.value(REG_EXTRACT_PRIMARY(reg));
		} else if (REG_IS_SECONDARY(reg)) {
			uint16_t r1(REG_EXTRACT_SECONDARY1(reg));
			uint16_t r2(REG_EXTRACT_SECONDARY2(reg));
			if (!qbrt_value::is_value_index(cfunc.value(r1))) {
				cerr << "cannot access secondary register: "
					<< r1 << endl;
				return *(qbrt_value *) NULL;
			}
			return cfunc.value(r1).data.reg->value(r2);
		}
		cerr << "Unsupported ref register: " << reg << endl;
		return *(qbrt_value *) NULL;
	}

	Failure * failure(uint16_t reg)
	{
		if (REG_IS_PRIMARY(reg)) {
			uint16_t r(REG_EXTRACT_PRIMARY(reg));
			return NULL;
		} else if (REG_IS_SECONDARY(reg)) {
			uint16_t r1(REG_EXTRACT_SECONDARY1(reg));
			uint16_t r2(REG_EXTRACT_SECONDARY2(reg));
		}
		return NULL;
	}

	qbrt_value * get_context(const std::string &name)
	{
		return add_context(&frame, name);
	}

	const ResourceTable & resource() const
	{
		return *(const ResourceTable *) NULL;
	}

	virtual void io(StreamIO *op)
	{
		frame.io_push(op);
	}

private:
	Worker &w;
	CodeFrame &frame;
	function_value &cfunc;
};


void execute_binaryop(OpContext &ctx, const binaryop_instruction &i)
{
	const qbrt_value *a;
	const qbrt_value *b;
	READ_REG(a, ctx, i.a);
	READ_REG(b, ctx, i.b);

	Failure *fail;
	qbrt_value *result(ctx.dstvalue(i.result));
	switch (i.opcode()) {
		case OP_IADD:
		case OP_ISUB:
		case OP_IMULT:
			if (a->type->id != VT_INT) {
				fail = FAIL_TYPE(ctx.module_name(),
						ctx.function_name(), ctx.pc());
				fail->debug << "unexpected type for first "
					"operand in integer binary operation: "
					<< (int) a->type->id;
				qbrt_value::fail(*result, fail);
				ctx.pc() += binaryop_instruction::SIZE;
				return;
			}
			if (b->type->id != VT_INT) {
				fail = FAIL_TYPE(ctx.module_name(),
						ctx.function_name(), ctx.pc());
				fail->debug << "unexpected type for second "
					"operand in integer binary operation: "
					<< (int) b->type->id;
				qbrt_value::fail(*result, fail);
				ctx.pc() += binaryop_instruction::SIZE;
				return;
			}
			break;
		default:
			cerr << "how'd *that* happen?\n";
			exit(1);
	}
	switch (i.opcode()) {
		case OP_IADD:
			qbrt_value::i(*result, a->data.i + b->data.i);
			break;
		case OP_ISUB:
			qbrt_value::i(*result, a->data.i - b->data.i);
			break;
		case OP_IMULT:
			qbrt_value::i(*result, a->data.i * b->data.i);
			break;
	}
	ctx.pc() += binaryop_instruction::SIZE;
}

void execute_divide(OpContext &ctx, const binaryop_instruction &i)
{
	const qbrt_value *a;
	const qbrt_value *b;
	READ_REG(a, ctx, i.a);
	READ_REG(b, ctx, i.b);

	qbrt_value &result(*ctx.dstvalue(i.result));
	if (b->data.i == 0) {
		qbrt_value::fail(result, NEW_FAILURE("divideby0"
				, ctx.module_name(), ctx.function_name()
				, ctx.pc()));
	} else {
		qbrt_value::i(result, a->data.i / b->data.i);
	}
	ctx.pc() += binaryop_instruction::SIZE;
}

void execute_fieldget(OpContext &ctx, const fieldget_instruction &i)
{
	const qbrt_value *src;
	qbrt_value *dst;
	READ_REG(src, ctx, i.src);
	WRITE_REG(dst, ctx, i.dst);

	const ResourceTable &resource(ctx.resource());
	const char *field_name = fetch_string(resource, i.field_name);

	int16_t fldidx(src->get_field_index(field_name));
	if (fldidx < 0) {
		cerr << "could not retrieve field named: " << field_name <<endl;
		exit(1);
	}

	qbrt_value::copy(*dst, src->data.reg->value(fldidx));
	ctx.pc() += fieldget_instruction::SIZE;
}

void execute_fieldset(OpContext &ctx, const fieldset_instruction &i)
{
	const qbrt_value *src;
	qbrt_value *dst;
	READ_REG(src, ctx, i.src);
	WRITE_REG(dst, ctx, i.dst);

	const ResourceTable &resource(ctx.resource());
	const char *field_name = fetch_string(resource, i.field_name);
	int16_t fldidx(src->get_field_index(field_name));
	if (fldidx < 0) {
		cerr << "could not retrieve field named: " << field_name <<endl;
		exit(1);
	}

	qbrt_value::copy(dst->data.reg->value(fldidx), *src);
	ctx.pc() += fieldset_instruction::SIZE;
}

void execute_fork(OpContext &ctx, const fork_instruction &i)
{
	Worker &w(ctx.worker());
	CodeFrame &parent(*w.current);
	CodeFrame *child(fork_frame(parent));
	child->pc = parent.pc + fork_instruction::SIZE;
	w.fresh->push_back(child);

	qbrt_value &fork_target(*ctx.dstvalue(i.result));
	qbrt_value::promise(fork_target, new Promise(w.id));
	ctx.pc() += i.jump();
}

void execute_goto(OpContext &ctx, const goto_instruction &i)
{
	ctx.pc() += i.jump();
}

/**
 * If the condition is true, keep executing. else jump to the label
 */
void execute_if(OpContext &ctx, const if_instruction &i)
{
	const qbrt_value *op;
	READ_REG(op, ctx, i.op);

	if (i.opcode() == OP_IF && op->data.b
			|| i.opcode() == OP_IFNOT && ! op->data.b) {
		ctx.pc() += if_instruction::SIZE;
	} else {
		ctx.pc() += i.jump();
	}
}

bool equal_value(const qbrt_value &a, const qbrt_value &b)
{
	if (a.type != b.type) {
		return false;
	}
	switch (a.type->id) {
		case VT_INT:
			return a.data.i == b.data.i;
		case VT_HASHTAG:
			return *a.data.hashtag == *b.data.hashtag;
		case VT_LIST:
		case VT_CONSTRUCT:
			return *a.data.cons == *b.data.cons;
		default:
			cerr << "unsupported type comparison: "
				<< (int) a.type->id << endl;
			break;
	}
	return false;
}

/**
 * If failure, keep going. If not failure, jump to the given label
 */
void execute_iffail(OpContext &ctx, const iffail_instruction &i)
{
	const qbrt_value *op;
	READ_FAILED_REG(op, ctx, i.op);

	bool is_failure(op->type->id == TYPE_FAILURE.id);
	int valtype(op->type->id);
	if (i.opcode() == OP_IFFAIL && is_failure
			|| i.opcode() == OP_IFNOTFAIL && ! is_failure) {
		ctx.pc() += iffail_instruction::SIZE;
	} else {
		ctx.pc() += i.jump();
	}
}

void execute_cfailure(OpContext &ctx, const cfailure_instruction &i)
{
	qbrt_value *result;
	WRITE_REG(result, ctx, i.dst);

	const char *failtype = fetch_string(ctx.resource(), i.hashtag_id);
	Failure *f = NEW_FAILURE(failtype, ctx.module_name()
			, ctx.function_name(), ctx.pc());
	ctx.backtrace(*f);
	qbrt_value::fail(*result, f);
	ctx.pc() += cfailure_instruction::SIZE;
}

void execute_cmp(OpContext &ctx, const cmp_instruction &i)
{
	const qbrt_value *a;
	const qbrt_value *b;
	qbrt_value *dst;
	READ_REG(a, ctx, i.a);
	READ_REG(b, ctx, i.b);
	WRITE_REG(dst, ctx, i.result);

	Failure *f;
	int comparison(qbrt_compare(*a, *b));
	bool result;
	switch (i.opcode()) {
		case OP_CMP_EQ:
			qbrt_value::b(*dst, comparison == 0);
			break;
		case OP_CMP_NOTEQ:
			qbrt_value::b(*dst, comparison != 0);
			break;
		case OP_CMP_GT:
			qbrt_value::b(*dst, comparison > 0);
			break;
		case OP_CMP_GTEQ:
			qbrt_value::b(*dst, comparison >= 0);
			break;
		case OP_CMP_LT:
			qbrt_value::b(*dst, comparison < 0);
			break;
		case OP_CMP_LTEQ:
			qbrt_value::b(*dst, comparison >= 0);
			break;
		default:
			f = NEW_FAILURE("invalidcmpop", ctx.module_name()
					, ctx.function_name()
					, ctx.pc());
			ctx.backtrace(*f);
			ctx.fail_frame(f);
			return;
	}
	ctx.pc() += cmp_instruction::SIZE;
}

void execute_consti(OpContext &ctx, const consti_instruction &i)
{
	qbrt_value *dst;
	WRITE_REG(dst, ctx, i.reg);

	qbrt_value::i(*dst, i.value);
	ctx.pc() += consti_instruction::SIZE;
}

void execute_move(OpContext &ctx, const move_instruction &i)
{
	const qbrt_value *src;
	qbrt_value *dst;
	READ_REG(src, ctx, i.src);
	WRITE_REG(dst, ctx, i.dst);

	*dst = *src;
	ctx.pc() += move_instruction::SIZE;
}

void execute_ref(OpContext &ctx, const ref_instruction &i)
{
	qbrt_value &dst(ctx.refvalue(i.dst));
	qbrt_value &src(ctx.refvalue(i.src));
	qbrt_value::ref(dst, src);
	ctx.pc() += ref_instruction::SIZE;
}

void execute_copy(OpContext &ctx, const copy_instruction &i)
{
	const qbrt_value *src;
	qbrt_value *dst;
	READ_REG(src, ctx, i.src);
	WRITE_REG(dst, ctx, i.dst);

	*dst = *src;
	ctx.pc() += copy_instruction::SIZE;
}

void execute_consts(OpContext &ctx, const consts_instruction &i)
{
	qbrt_value *dst;
	WRITE_REG(dst, ctx, i.reg);

	const char *str = fetch_string(ctx.resource(), i.string_id);
	qbrt_value::str(*dst, str);
	ctx.pc() += consts_instruction::SIZE;
}

void execute_consthash(OpContext &ctx, const consthash_instruction &i)
{
	qbrt_value *dst;
	WRITE_REG(dst, ctx, i.reg);

	const char *hash = fetch_string(ctx.resource(), i.hash_id);
	qbrt_value::hashtag(*dst, hash);
	ctx.pc() += consthash_instruction::SIZE;
}

void execute_ctuple(OpContext &ctx, const ctuple_instruction &i)
{
	qbrt_value *dst;
	WRITE_REG(dst, ctx, i.dst);

	qbrt_value::tuple(*dst, new Tuple(i.size));
	ctx.pc() += ctuple_instruction::SIZE;
}

void execute_lcontext(OpContext &ctx, const lcontext_instruction &i)
{
	qbrt_value *dst;
	WRITE_REG(dst, ctx, i.reg);

	const char *name = fetch_string(ctx.resource(), i.hashtag);
	Failure *fail;

	qbrt_value *src = ctx.get_context(name);
	if (src) {
		qbrt_value::ref(*dst, *src);
	} else {
		fail = NEW_FAILURE("unknown_context", ctx.module_name()
				, ctx.function_name(), ctx.pc());
		fail->debug << "cannot find context variable: " << name;
		qbrt_value::fail(*dst, fail);
		cerr << fail->debug_msg() << endl;
	}

	ctx.pc() += lcontext_instruction::SIZE;
}

void execute_lconstruct(OpContext &ctx, const lconstruct_instruction &i)
{
	const ResourceTable &resource(ctx.resource());
	const FullId cons(fetch_fullid(resource, i.modsym));
	const Module *mod(find_module(ctx.worker(), cons.module));

	Failure *fail;
	qbrt_value *dst(ctx.dstvalue(i.reg));
	if (!dst) {
		fail = FAIL_REGISTER404(ctx.module_name(), ctx.function_name()
				, ctx.pc());
		fail->debug << "invalid register: " << i.reg;
		ctx.fail_frame(fail);
		return;
	}

	if (mod) {
		Module::load_construct(*dst, *mod, cons.id);
	} else {
		fail = FAIL_MODULE404(ctx.module_name(), ctx.function_name()
				, ctx.pc());
		fail->debug << "Cannot find module: '" << cons.module << "'";
		qbrt_value::fail(*dst, fail);
		// continue on with execution
	}

	ctx.pc() += lcontext_instruction::SIZE;
}

void execute_loadfunc(OpContext &ctx, const lfunc_instruction &i)
{
	const ResourceTable &resource(ctx.resource());
	const ModSym &modsym(fetch_modsym(resource, i.modsym));
	const char *modname = fetch_string(resource, modsym.mod_name());
	const char *fname = fetch_string(resource, modsym.sym_name());
	const Module *mod(find_module(ctx.worker(), modname));
	Failure *fail;

	qbrt_value *dst(ctx.dstvalue(i.reg));
	if (!dst) {
		fail = FAIL_REGISTER404(ctx.module_name(), ctx.function_name()
				, ctx.pc());
		fail->debug << "Invalid register: " << i.reg;
		ctx.fail_frame(fail);
		return;
	}

	if (!mod) {
		fail = FAIL_MODULE404(ctx.module_name(), ctx.function_name()
				, ctx.pc());
		fail->debug << "Cannot find module: '" << modname << "'";
		qbrt_value::fail(*dst, fail);
		ctx.pc() += lfunc_instruction::SIZE;
		return;
	}
	const QbrtFunction *qbrt(mod->fetch_function(fname));

	c_function cf = NULL;
	if (qbrt) {
		qbrt_value::f(*dst, new function_value(qbrt));
	} else {
		const CFunction *cf = fetch_c_function(*mod, fname);
		if (cf) {
			qbrt_value::f(*dst, new function_value(cf));
		} else {
			fail = FAIL_FUNCTION404(ctx.module_name()
					, ctx.function_name(), ctx.pc());
			fail->debug << "could not find function: " << modname
				<<'.'<< fname;
			qbrt_value::fail(*dst, fail);
		}
	}
	ctx.pc() += lfunc_instruction::SIZE;
}

void execute_match(OpContext &ctx, const match_instruction &i)
{
	qbrt_value &result(*ctx.dstvalue(i.result));
	qbrt_value &pattern(*ctx.dstvalue(i.pattern));
	qbrt_value &input(*ctx.dstvalue(i.input));

	int comparison(qbrt_compare(pattern, input));
	bool match(comparison == 0);
	if (match) {
		qbrt_value::copy(result, input);
		ctx.pc() += match_instruction::SIZE;
	} else {
		ctx.pc() += i.jump();
	}
}

void execute_matchargs(OpContext &ctx, const matchargs_instruction &i)
{
	const qbrt_value &pattern_val(*ctx.srcvalue(i.pattern));
	qbrt_value &result_val(*ctx.dstvalue(i.result));
	if (!qbrt_value::is_value_index(pattern_val)) {
		cerr << "invalid pattern argument in matchargs\n";
		ctx.pc() += i.jump();
		return;
	}
	if (!qbrt_value::is_value_index(result_val)) {
		cerr << "invalid result argument in matchargs\n";
		ctx.pc() += i.jump();
		return;
	}
	const qbrt_value_index &pattern(*pattern_val.data.reg);
	qbrt_value_index &result(*result_val.data.reg);

	int cmp;
	int argc(ctx.argc());
	int j;
	for (j=0; j<argc; ++j) {
		cmp = qbrt_compare(pattern.value(j)
				, *ctx.srcvalue(PRIMARY_REG(j)));
		if (cmp != 0) {
			ctx.pc() += i.jump();
			return;
		}
	}

	for (j=0; j<argc; ++j) {
		if (pattern.value(j).type->id == VT_PATTERNVAR) {
			qbrt_value::copy(result.value(j)
					, *ctx.srcvalue(PRIMARY_REG(j)));
		}
	}
	ctx.pc() += matchargs_instruction::SIZE;
}

void execute_newproc(OpContext &ctx, const newproc_instruction &i)
{
	Failure *f;
	qbrt_value &pid(*ctx.dstvalue(i.pid));
	qbrt_value *func(ctx.dstvalue(i.func));

	if (func->type->id != VT_FUNCTION) {
		f = FAIL_TYPE(ctx.module_name(), ctx.function_name(), ctx.pc());
		qbrt_value::fail(pid, f);
		ctx.pc() += newproc_instruction::SIZE;
		return;
	}

	ctx.pc() += newproc_instruction::SIZE;

	function_value *fval = func->data.f;
	qbrt_value::set_void(*func);

	Worker &w(ctx.worker());

	const QbrtFunction *qfunc;
	qfunc = dynamic_cast< const QbrtFunction * >(fval->func);
	FunctionCall *call = new FunctionCall(*qfunc, *fval);
	ProcessRoot *proc = new_process(w.app, call);
	qbrt_value::i(pid, proc->pid);
}

void execute_patternvar(OpContext &ctx, const patternvar_instruction &i)
{
	Failure *f;
	qbrt_value *dst(ctx.dstvalue(i.dst));
	if (!dst) {
		cerr << "invalid register for patternvar: " << i.dst << endl;
		return;
	}

	qbrt_value::patternvar(*dst);
	ctx.pc() += patternvar_instruction::SIZE;
}

void execute_recv(OpContext &ctx, const recv_instruction &i)
{
	Worker &w(ctx.worker());
	if (w.current->proc->recv.empty()) {
		w.current->cfstate = CFS_PEERWAIT;
		return;
	}

	qbrt_value &dst(*ctx.dstvalue(i.dst));
	qbrt_value *msg(w.current->proc->recv.pop());
	dst = *msg;
	ctx.pc() += recv_instruction::SIZE;
}

void execute_stracc(OpContext &ctx, const stracc_instruction &i)
{
	const qbrt_value *src;
	READ_REG(src, ctx, i.src);
	RETURN_FAILURE(ctx, i.dst);

	Failure *f;
	qbrt_value *dst(ctx.dstvalue(i.dst));

	int op_pc(ctx.pc());
	ctx.pc() += stracc_instruction::SIZE;

	if (dst->type->id != VT_STRING) {
		f = FAIL_TYPE(ctx.module_name(), ctx.function_name(), op_pc);
		f->debug << "stracc destination is not a string";
		qbrt_value::i(f->exit_code, 1);
		ctx.fail_frame(f);
		return;
	}

	ostringstream out;
	switch (src->type->id) {
		case VT_STRING:
			*dst->data.str += *src->data.str;
			break;
		case VT_INT:
			out << src->data.i;
			*dst->data.str += out.str();
			break;
		case VT_VOID:
			f = FAIL_TYPE(ctx.module_name(), ctx.function_name()
					, op_pc);
			f->debug << "cannot append void to string";
			cerr << f->debug_msg() << endl;
			qbrt_value::fail(*dst, f);
			break;
		default:
			f = FAIL_TYPE(ctx.module_name(), ctx.function_name()
					, op_pc);
			f->debug << "stracc source type is not supported: "
				<< (int) src->type->id;
			cerr << f->debug_msg() << endl;
			qbrt_value::fail(*dst, f);
			break;
	}
}

void execute_loadtype(OpContext &ctx, const loadtype_instruction &i)
{
	const char *modname = fetch_string(ctx.resource(), i.modname);
	const char *type_name = fetch_string(ctx.resource(), i.type);
	const Module &mod(*find_module(ctx.worker(), modname));
	const Type *typ = NULL; // mod.fetch_struct(type_name);
	qbrt_value &dst(*ctx.dstvalue(i.reg));
	qbrt_value::typ(dst, typ);
	ctx.pc() += loadtype_instruction::SIZE;
}

void execute_loadobj(OpContext &ctx, const loadobj_instruction &i)
{
	const char *modname = fetch_string(ctx.resource(), i.modname);
	load_module(ctx.worker(), modname);
	ctx.pc() += loadobj_instruction::SIZE;
}

void call(Worker &ctx, qbrt_value &res, qbrt_value &f);

void execute_call(OpContext &ctx, const call_instruction &i)
{
	qbrt_value *output;
	WRITE_REG(output, ctx, i.result_reg);

	qbrt_value &func_reg(*ctx.dstvalue(i.func_reg));

	// increment pc so it's in the right place when we get back
	ctx.pc() += call_instruction::SIZE;
	call(ctx.worker(), *output, func_reg);
}

void execute_return(OpContext &ctx, const return_instruction &i)
{
	Worker &w(ctx.worker());
	w.current->cfstate = CFS_COMPLETE;
}

void fail(OpContext &ctx, Failure *f)
{
	qbrt_value &result(*ctx.dstvalue(SPECIAL_REG_RESULT));
	qbrt_value::fail(result, f);

	Worker &w(ctx.worker());
	w.current->cfstate = CFS_FAILED;
	// where to set the failure?
}

typedef void (*executioner)(OpContext &, const instruction &);
executioner EXECUTIONER[NUM_OP_CODES] = {0};

void init_executioners()
{
	executioner *x = EXECUTIONER;

	x[OP_CALL] = (executioner) execute_call;
	x[OP_RETURN] = (executioner) execute_return;
	x[OP_CFAILURE] = (executioner) execute_cfailure;
	x[OP_CMP_EQ] = (executioner) execute_cmp;
	x[OP_CMP_NOTEQ] = (executioner) execute_cmp;
	x[OP_CMP_GT] = (executioner) execute_cmp;
	x[OP_CMP_GTEQ] = (executioner) execute_cmp;
	x[OP_CMP_LT] = (executioner) execute_cmp;
	x[OP_CMP_LTEQ] = (executioner) execute_cmp;
	x[OP_CONSTI] = (executioner) execute_consti;
	x[OP_CONSTS] = (executioner) execute_consts;
	x[OP_CONSTHASH] = (executioner) execute_consthash;
	x[OP_CTUPLE] = (executioner) execute_ctuple;
	x[OP_FIELDGET] = (executioner) execute_fieldget;
	x[OP_FIELDSET] = (executioner) execute_fieldset;
	x[OP_IADD] = (executioner) execute_binaryop;
	x[OP_IDIV] = (executioner) execute_divide;
	x[OP_IMULT] = (executioner) execute_binaryop;
	x[OP_ISUB] = (executioner) execute_binaryop;
	x[OP_LCONTEXT] = (executioner) execute_lcontext;
	x[OP_LCONSTRUCT] = (executioner) execute_lconstruct;
	x[OP_LFUNC] = (executioner) execute_loadfunc;
	x[OP_MATCH] = (executioner) execute_match;
	x[OP_MATCHARGS] = (executioner) execute_matchargs;
	x[OP_NEWPROC] = (executioner) execute_newproc;
	x[OP_PATTERNVAR] = (executioner) execute_patternvar;
	x[OP_RECV] = (executioner) execute_recv;
	x[OP_STRACC] = (executioner) execute_stracc;
	x[OP_LOADOBJ] = (executioner) execute_loadobj;
	x[OP_MOVE] = (executioner) execute_move;
	x[OP_REF] = (executioner) execute_ref;
	x[OP_COPY] = (executioner) execute_copy;
	x[OP_FORK] = (executioner) execute_fork;
	x[OP_GOTO] = (executioner) execute_goto;
	x[OP_IF] = (executioner) execute_if;
	x[OP_IFNOT] = (executioner) execute_if;
	x[OP_IFFAIL] = (executioner) execute_iffail;
	x[OP_IFNOTFAIL] = (executioner) execute_iffail;
}

void execute_instruction(Worker &w, const instruction &i)
{
	uint8_t opcode(i.opcode());
	executioner x = EXECUTIONER[opcode];
	WorkerOpContext ctx(w);
	if (!x) {
		Failure *f = NEW_FAILURE("invalidopcode", ctx.module_name()
				, ctx.function_name(), ctx.pc());
		qbrt_value::i(f->exit_code, 1);
		f->debug << "Opcode not implemented: " << (int) opcode;
		f->usage << "Internal program error";
		ctx.backtrace(*f);
		ctx.fail_frame(f);
		return;
	}
	//cerr << "---------------------------\n";
	//cerr << "execute opcode: " << (int) opcode << endl;
	x(ctx, i);
	//inspect_call_frame(cerr, *w.task->cframe);
}

void override_function(Worker &w, function_value &funcval)
{
	int pfc_type(PFC_TYPE(funcval.fcontext()));
	if (pfc_type == FCT_TRADITIONAL) {
		// no overrides for regular functions
		return;
	}

	if (pfc_type == FCT_POLYMORPH) {
		// no override. reset the func to the protocol function
		// if it was previously overridden
		const Function *def_func =
			find_default_function(w, *funcval.func);
		reassign_func(funcval, def_func);
	}

	ostringstream value_type_stream;
	load_function_value_types(value_type_stream, funcval);
	string value_types(value_type_stream.str());

	const Function &func(*funcval.func);
	const QbrtFunction *qfunc;
	qfunc = dynamic_cast< const QbrtFunction * >(&func);
	if (!qfunc) {
		cerr << "not a qbrt_function_value\n";
		return;
	}

	const char *proto_name = func.protocol_name();
	const QbrtFunction *overridef(find_override(w, func.mod->name.c_str()
				, proto_name, funcval.name(), value_types));
	if (overridef) {
		reassign_func(funcval, overridef);
	} else {
		const CFunction *cfunc = find_c_override(w
				, func.mod->name.c_str()
				, proto_name, funcval.name(), value_types);
		if (cfunc) {
			reassign_func(funcval, cfunc);
		}
	}
}

void qbrtcall(Worker &w, qbrt_value &res, function_value *f)
{
	if (!f) {
		cerr << "function is null\n";
		w.current->cfstate = CFS_FAILED;
		return;
	}

	// check that none of the function args are bad first
	WorkerCContext failctx(w, *f);
	for (uint16_t i(0); i<f->argc; ++i) {
		qbrt_value *val(failctx.dstvalue(PRIMARY_REG(i)));
		if (val->type->id == VT_FAILURE) {
			FunctionCall &failed_call(w.current->function_call());
			Failure *fail = val->data.failure;
			fail->trace_down(failed_call.mod->name
					, failed_call.name(), w.current->pc
					, __FILE__, __LINE__);
			qbrt_value::fail(*failed_call.result, fail);
			w.current->cfstate = CFS_FAILED;
			return;
		}
	}

	override_function(w, *f);

	if (f->abstract()) {
		// can't execute the function if it's abstract
		ostringstream types;
		load_function_value_types(types, *f);
		cerr << "cannot execute abstract function: "
			<< f->name() << "; " << types.str() << endl;
		w.current->cfstate = CFS_FAILED;
		return;
	}

	WorkerCContext ctx(w, *f);
	if (f->func->cfunc()) {
		c_function cf = f->func->cfunc();
		cf(ctx, res);
		return;
	}

	const QbrtFunction *qfunc;
	qfunc = dynamic_cast< const QbrtFunction * >(f->func);
	const ResourceTable &resource(f->func->mod->resource);
	// check arguments
	for (uint16_t i(0); i < f->argc; ++i) {
		const qbrt_value *val(ctx.srcvalue(PRIMARY_REG(i)));
		if (!val) {
			cerr << "wtf null value?\n";
		}
		const Type *valtype = val->type;
		const ParamResource &param(qfunc->header->params[i]);
		const char *name = fetch_string(resource, param.name_idx());
		const TypeSpecResource &type(
			resource.obj< TypeSpecResource >(param.type_idx()));
		const ModSym &type_ms(fetch_modsym(resource, type.name_idx()));
		const char *type_mod =
			fetch_string(resource, type_ms.mod_name());
		// */anything means it's a type variable. it's fine so proceed
		if (type_mod[0] == '*' && type_mod[1] == '\0') {
			continue;
		}
		const char *type_name =
			fetch_string(resource, type_ms.sym_name());
		if (valtype->module != type_mod || valtype->name != type_name) {
			cerr << "Type Mismatch: parameter " << name << '/' << i
				<< " expected to be " << type_mod << '/'
				<< type_name << ", instead received "
				<< valtype->module << '/' << valtype->name
				<< " " << w.current->function_call().name()
				<< ':' << w.current->pc << endl;
			exit(1);
		}
	}

	FunctionCall *call = new FunctionCall(*w.current, res, *qfunc, *f);
	w.current = call;
}

void call(Worker &w, qbrt_value &res, qbrt_value &f)
{
	Failure *fail;
	switch (f.type->id) {
		case VT_FUNCTION:
			qbrtcall(w, res, f.data.f);
			break;
		case VT_FAILURE:
			f.data.failure->trace_down(
					w.current->function_call().mod->name,
					w.current->function_call().name(),
					w.current->pc,
					__FILE__, __LINE__);
			qbrt_value::fail(res, f.data.failure);
			break;
		default:
			fail = FAIL_TYPE(w.current->function_call().mod->name
					, w.current->function_call().name()
					, w.current->pc);
			fail->debug << "Unknown function type: "
				<< (int)f.type->id;
			qbrt_value::fail(res, fail);
			return;
	}
}

void core_print(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = ctx.srcvalue(PRIMARY_REG(0));
	if (!val) {
		cout << "no param for print\n";
		return;
	}
	switch (val->type->id) {
		case VT_INT:
			cout << val->data.i;
			break;
		case VT_STRING:
			cout << *(val->data.str);
			break;
		default:
			cout << "type not supported by print: "
				<< val->type->name << endl;
			break;
	}
}

ostream & inspect_value_index(ostream &out, const qbrt_value_index &v)
{
	out << "value index:\n";
	inspect_indent += 2;
	for (int i(0); i<v.num_values(); ++i) {
		for (int j(0); j<inspect_indent; ++j) {
			out << "  ";
		}
		out << "v[" << i << "]:";
		const qbrt_value &val(v.value(i));
		inspect(out, val);
		out << endl;
	}
	inspect_indent -= 2;
	return out;
}

ostream & inspect_function(ostream &out, const Function &f)
{
	out << "function " << f.name() << "/" << (int) f.argc()
		<< '+' << (int) f.regc() << endl;
	out << "module: " << f.mod->name << endl;
	out << "name/fcontext: " << f.name() << '/'
		<< (int) f.fcontext()
		<< endl;
	return out;
}

ostream & inspect_call_frame(ostream &out, const CodeFrame &cf)
{
	const FunctionCall &fc(cf.function_call());
	// inspect_function(out, cf.func);
	const instruction &i(frame_instruction(cf));
	out << "pc: " << cf.pc << " => " << (int) i.opcode() << endl;
	out << "result: ";
	inspect(out, *fc.result);
	out << endl;
	inspect_value_index(out, cf);
	return out;
}

ostream & inspect_failure(ostream &out, const Failure &f)
{
	out << "Failure(#" << f.typestr() << ")";
	return out;
}

ostream & inspect_function_value(ostream &out, const function_value &fv)
{
	inspect_function(out, *fv.func);
	inspect_value_index(out, fv);
	return out;
}

ostream & inspect_ref(ostream &out, qbrt_value &ref)
{
	out << "ref:";
	qbrt_value *val(follow_ref(&ref));
	return inspect(out, *val);
}

ostream & inspect(ostream &out, const qbrt_value &v)
{
	switch (v.type->id) {
		case VT_VOID:
			out << "void";
			break;
		case VT_INT:
			out << "int:" << v.data.i;
			break;
		case VT_STRING:
			out << *v.data.str;
			break;
		case VT_FAILURE:
			inspect_failure(out, *v.data.failure);
			break;
		case VT_FUNCTION:
			inspect_function_value(out, *v.data.f);
			break;
		case VT_HASHTAG:
			out << '#' << *v.data.hashtag;
			break;
		case VT_BOOL:
			out << (v.data.b ? "true" : "false");
			break;
		case VT_FLOAT:
			out << v.data.fp;
			break;
		case VT_REF:
			inspect_ref(out, *v.data.ref);
			break;
		case VT_STREAM:
			out << "stream";
			break;
	}
	return out;
}

/**
 * Return the type of the input
 */
void get_qbrt_type(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = ctx.srcvalue(0);
	if (!val) {
		cerr << "no param for qbrt_type\n";
		return;
	}
	Type *t = NULL;
	switch (val->type->id) {
		case VT_VOID:
			t = &TYPE_VOID;
			break;
		case VT_INT:
			t = &TYPE_INT;
			break;
		case VT_STRING:
			t = &TYPE_STRING;
			break;
		case VT_BOOL:
			t = &TYPE_BOOL;
			break;
		case VT_FUNCTION:
			// need to arrange this somehow, but now sure how yet
			break;
		case VT_FLOAT:
			t = &TYPE_FLOAT;
			break;
		case VT_LIST:
			cerr << "it's a list. but of what?\n";
			break;
	}
	if (!t) {
		cerr << "wtf, where's my type?\n";
		return;
	}
	qbrt_value::typ(out, t);
}

void core_pid(OpContext &ctx, qbrt_value &result)
{
	qbrt_value::i(result, ctx.worker().current->proc->pid);
}

/** Return the worker ID to a result */
void core_wid(OpContext &ctx, qbrt_value &result)
{
	qbrt_value::i(result, ctx.worker().id);
}

void core_send(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value &pid(*ctx.srcvalue(PRIMARY_REG(0)));
	const qbrt_value &src(*ctx.srcvalue(PRIMARY_REG(1)));

	// check this worker first, to avoid any locking drama
	// when they're both on the same worker
	map< uint64_t, ProcessRoot * >::const_iterator it;
	Worker &w(ctx.worker());
	it = w.process.find(pid.data.i);
	if (it != w.process.end()) {
		it->second->recv.push(qbrt_value::dup(src));
		return;
	}

	bool success(send_msg(w.app, pid.data.i, src));
	if (!success) {
		cerr << "no process for pid " << pid.data.i << " on worker "
			<< w.id << endl;
	}
}

/** Convert a string value to a string, straight copy */
void core_str_from_str(OpContext &ctx, qbrt_value &result)
{
	qbrt_value::copy(result, *ctx.srcvalue(PRIMARY_REG(0)));
}

/** Convert an integer value to a string */
void core_str_from_int(OpContext &ctx, qbrt_value &result)
{
	const qbrt_value &src(*ctx.srcvalue(PRIMARY_REG(0)));
	ostringstream out;
	out << src.data.i;
	qbrt_value::str(result, out.str());
}

void list_empty(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = ctx.srcvalue(PRIMARY_REG(0));
	if (!val) {
		cerr << "no param for list empty\n";
		return;
	}
	if (val->type->id != VT_CONSTRUCT) {
		cerr << "empty arg not a list: " << (int) val->type->id
			<< endl;
	}
	List::is_empty(out, *val);
}

void list_head(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = ctx.srcvalue(PRIMARY_REG(0));
	List::head(out, *val);
}

void list_pop(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = ctx.srcvalue(PRIMARY_REG(0));
	if (!val) {
		cerr << "no param for list pop\n";
		return;
	}
}

void core_open(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value &filename(*ctx.srcvalue(PRIMARY_REG(0)));
	const qbrt_value &mode(*ctx.srcvalue(PRIMARY_REG(1)));
	// these type checks should be done automatically...
	// once types are working.
	if (filename.type->id != VT_STRING) {
		cerr << "first argument to open is not a string\n";
		cerr << "argument is type: " << (int)filename.type->id << endl;
		exit(2);
	}
	if (mode.type->id != VT_STRING) {
		cerr << "second argument to open is not a string\n";
		cerr << "argument is type: " << (int) mode.type->id << endl;
		exit(2);
	}
	FILE *f = fopen(filename.data.str->c_str(), mode.data.str->c_str());
	int fd = fileno(f);
	qbrt_value::stream(out, new FileStream(fd, f));
}

void core_getline(OpContext &ctx, qbrt_value &out)
{
	qbrt_value &stream(*ctx.dstvalue(PRIMARY_REG(0)));
	if (stream.type->id != VT_STREAM) {
		cerr << "first argument to getline is not a stream\n";
		exit(2);
	}
	ctx.io(stream.data.stream->getline(out));
}

void core_write(OpContext &ctx, qbrt_value &out)
{
	qbrt_value &stream(*ctx.dstvalue(PRIMARY_REG(0)));
	const qbrt_value &text(*ctx.srcvalue(PRIMARY_REG(1)));
	if (stream.type->id != VT_STREAM) {
		cerr << "first argument to write is not a stream\n";
		exit(2);
	}
	if (text.type->id != VT_STRING) {
		cerr << "second argument to write is not a string\n";
		cerr << "argument is type: " << (int) text.type->id << endl;
		exit(2);
	}
	ctx.io(stream.data.stream->write(*text.data.str));
}

Module * load_core_module(Application &app)
{
	Module *mod_core = const_cast< Module * >(load_module(app, "core"));
	if (!mod_core) {
		return NULL;
	}

	add_c_function(*mod_core, core_pid, "pid", 0, "");
	add_c_function(*mod_core, core_send, "send", 2
			, "io/Stream;core/String;");
	add_c_function(*mod_core, core_wid, "wid", 0, "");
	add_type(*mod_core, "Int", TYPE_INT);
	add_type(*mod_core, "String", TYPE_STRING);
	add_type(*mod_core, "ByteString", TYPE_STRING);
	add_c_override(*mod_core, core_str_from_str, "core", "Stringy", "str", 1
			, "core/String");
	add_c_override(*mod_core, core_str_from_int, "core", "Stringy", "str", 1
			, "core/Int");
	return mod_core;
}

Module * load_io_module(Application &app)
{
	Module *mod_io = new Module("io");
	add_c_function(*mod_io, core_print, "print", 1, "core/String;");
	add_c_function(*mod_io, core_open, "open", 2
			, "core/String;core/String;");
	add_c_function(*mod_io, core_write, "write", 2
			, "io/Stream;core/String;");
	add_c_function(*mod_io, core_getline, "getline", 1, "io/Stream;");
	load_module(app, mod_io);
	return mod_io;
}

Module * load_list_module(Application &app)
{
	Module *mod_list = const_cast< Module * >(load_module(app, "list"));
	if (!mod_list) {
		return NULL;
	}
	add_c_function(*mod_list, list_empty, "is_empty", 1, "core/List;");
	add_c_function(*mod_list, list_head, "head", 1, "core/List;");
	add_c_function(*mod_list, list_pop, "pop", 1, "core/List;");
	load_module(app, mod_list);
	return mod_list;
}

int main(int argc, const char **argv)
{
	if (argc < 2) {
		cerr << "an object name is required\n";
		return 0;
	}
	const char *objname = argv[1];
	init_executioners();
	init_const_registers();


	Application app;
	Module *mod_core(load_core_module(app));
	Module *mod_io(load_io_module(app));
	Module *mod_list(load_list_module(app));
	if (!(mod_core && mod_io && mod_list)) {
		return -1;
	}

	Worker &w0(new_worker(app));
	Worker &w1(new_worker(app));

	const Module *main_module = load_module(app, objname);
	if (!main_module) {
		return 1;
	}

	const QbrtFunction *qbrt_main(main_module->fetch_function("__main"));
	if (!qbrt_main) {
		cerr << "no __main function defined\n";
		return 1;
	}
	function_value *main_func = new function_value(qbrt_main);
	int main_func_argc(main_func->argc);
	if (main_func_argc >= 1) {
		qbrt_value::i(main_func->regv[0], argc - 1);
	}
	if (main_func_argc >= 2) {
		qbrt_value head;
		Module::load_construct(head, *mod_list, "Empty");
		for (int i(1); i<argc; ++i) {
			qbrt_value node;
			Module::load_construct(node, *mod_list, "Node");
			qbrt_value::str(node.data.reg->value(0), argv[i]);
			node.data.reg->value(1) = head;
			head = node;
		}
		List::reverse(main_func->regv[1], head);
	}

	Stream *stream_stdin = NULL;
	Stream *stream_stdout = NULL;
	struct stat buf;
	fstat(fileno(stdin), &buf);
	// printf("mode is %o\n", buf.st_mode);
	if (S_ISREG(buf.st_mode)) {
		// cerr << "stdin is a regular file\n";
		stream_stdin = new FileStream(fileno(stdin), stdin);
		stream_stdout = new FileStream(fileno(stdout), stdout);
	} else if (S_ISCHR(buf.st_mode)) {
		// cerr << "stdin is a character device\n";
		stream_stdin = new FileStream(fileno(stdin), stdin);
		stream_stdout = new FileStream(fileno(stdout), stdout);
	} else if (S_ISFIFO(buf.st_mode)) {
		// cerr << "stdin is fifo\n";
		stream_stdin = new ByteStream(fileno(stdin), stdin);
		stream_stdout = new ByteStream(fileno(stdout), stdout);
	}


	qbrt_value result;
	qbrt_value::i(result, 0);
	FunctionCall *main_call = new FunctionCall(result, *qbrt_main
			, *main_func);
	qbrt_value::stream(*add_context(main_call, "stdin"), stream_stdin);
	qbrt_value::stream(*add_context(main_call, "stdout"), stream_stdout);
	ProcessRoot *main_proc = new_process(app, main_call);

	int tid1 = pthread_create(&w0.thread, &w0.thread_attr
			, launch_worker, &w0);
	int tid2 = pthread_create(&w1.thread, &w1.thread_attr
			, launch_worker, &w1);

	application_loop(app);

	if (qbrt_value::failed(result)) {
		Failure *fail = result.data.failure;
		Failure::write(cerr, *fail);
		return fail->exit_code.data.i;
	}

	return result.data.i;
}
