
#include "qbrt/core.h"
#include "qbrt/schedule.h"
#include "qbrt/string.h"
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/logic.h"
#include "qbrt/tuple.h"
#include "qbrt/list.h"
#include "qbrt/map.h"
#include "qbrt/vector.h"
#include "qbrt/module.h"
#include "io.h"
#include "instruction/schedule.h"

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

#define CHECK_FAILURE(result, arg) do { \
	if (arg.type->id == VT_FAILURE) { \
		qbrt_value::fail(result, arg.data.failure); \
		return; \
	}} while (0)

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
	virtual uint8_t regc() const = 0;
	virtual const qbrt_value & srcvalue(uint16_t) const = 0;
	virtual qbrt_value & dstvalue(uint16_t) = 0;
	virtual qbrt_value & refvalue(uint16_t) = 0;
	virtual const char * function_name() const = 0;
	virtual int & pc() const = 0;
	virtual Worker & worker() const = 0;
	virtual const ResourceTable & resource() const = 0;
	virtual qbrt_value * get_context(const std::string &) = 0;
	virtual void io(StreamIO *op) = 0;
};

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
	virtual const char * function_name() const { return func.name(); }
	virtual int & pc() const { return frame.pc; }
	virtual uint8_t regc() const { return func.regc; }
	virtual const qbrt_value & srcvalue(uint16_t reg) const
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(func.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(func.value(primary)).data.reg
				->value(secondary);
		} else if (SPECIAL_REG_RESULT == reg) {
			return *func.result;
		} else if (REG_IS_CONST(reg)) {
			return CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf src reg? " << reg << endl;
		return *(qbrt_value *) NULL;
	}

	virtual qbrt_value & dstvalue(uint16_t reg)
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(func.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(func.value(primary)).data.reg
				->value(secondary);
		} else if (CONST_REG_VOID == reg) {
			return w.drain;
		} else if (SPECIAL_REG_RESULT == reg) {
			return *func.result;
		} else if (REG_IS_CONST(reg)) {
			return CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf dst reg? " << reg << endl;
		return *(qbrt_value *) NULL;
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
	WorkerCContext(Worker &w, cfunction_value &cf)
	: w(w)
	, frame(*w.current)
	, cfunc(cf)
	{}

	virtual Worker & worker() const { return w; }
	virtual const char * function_name() const { return "cfunction"; }
	virtual int & pc() const { return *(int *) NULL; }
	virtual uint8_t regc() const { return cfunc.regc; }
	virtual const qbrt_value & srcvalue(uint16_t reg) const
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(cfunc.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(cfunc.value(primary)).data.reg
				->value(secondary);
		} else if (SPECIAL_REG_RESULT == reg) {
			cerr << "no src result register for c functions\n";
			return *(qbrt_value *) NULL;
		} else if (REG_IS_CONST(reg)) {
			return CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf src reg? " << reg << endl;
		return *(qbrt_value *) NULL;
	}

	virtual qbrt_value & dstvalue(uint16_t reg)
	{
		uint8_t primary, secondary;
		if (REG_IS_PRIMARY(reg)) {
			primary = REG_EXTRACT_PRIMARY(reg);
			return follow_ref(cfunc.value(primary));
		} else if (REG_IS_SECONDARY(reg)) {
			primary = REG_EXTRACT_SECONDARY1(reg);
			secondary = REG_EXTRACT_SECONDARY2(reg);
			return follow_ref(cfunc.value(primary)).data.reg
				->value(secondary);
		} else if (CONST_REG_VOID == reg) {
			return w.drain;
		} else if (SPECIAL_REG_RESULT == reg) {
			cerr << "no dst result register for c functions\n";
			return *(qbrt_value *) NULL;
		} else if (REG_IS_CONST(reg)) {
			return CONST_REGISTER[REG_EXTRACT_CONST(reg)];
		}
		cerr << "wtf dst reg? " << reg << endl;
		return *(qbrt_value *) NULL;
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
	cfunction_value &cfunc;
};


void execute_binaryop(OpContext &ctx, const binaryop_instruction &i)
{
	const qbrt_value &a(ctx.srcvalue(i.a));
	const qbrt_value &b(ctx.srcvalue(i.b));
	qbrt_value &result(ctx.dstvalue(i.result));
	switch (i.opcode()) {
		case OP_IADD:
			qbrt_value::i(result, a.data.i + b.data.i);
			break;
		case OP_ISUB:
			qbrt_value::i(result, a.data.i - b.data.i);
			break;
		case OP_IMULT:
			qbrt_value::i(result, a.data.i * b.data.i);
			break;
	}
	ctx.pc() += binaryop_instruction::SIZE;
}

void execute_divide(OpContext &ctx, const binaryop_instruction &i)
{
	const qbrt_value &b(ctx.srcvalue(i.b));
	qbrt_value &result(ctx.dstvalue(i.result));
	if (b.data.i == 0) {
		qbrt_value::fail(result, new Failure("divideby0"));
	} else {
		const qbrt_value &a(ctx.srcvalue(i.a));
		qbrt_value::i(result, a.data.i / b.data.i);
	}
	ctx.pc() += binaryop_instruction::SIZE;
}

void execute_fork(OpContext &ctx, const fork_instruction &i)
{
	Worker &w(ctx.worker());
	CodeFrame &parent(*w.current);
	CodeFrame *child(fork_frame(parent));
	child->pc = parent.pc + fork_instruction::SIZE;
	w.fresh->push_back(child);

	qbrt_value &fork_target(ctx.dstvalue(i.result));
	qbrt_value::promise(fork_target, new Promise(w.id));
	ctx.pc() += i.jump();
}

void execute_wait(OpContext &ctx, const wait_instruction &i)
{
	Worker &w(ctx.worker());
	const qbrt_value &subject(ctx.srcvalue(i.reg));
	if (subject.type->id == VT_PROMISE) {
		w.current->cfstate = CFS_PEERWAIT;
		// wait right here, don't change the pc
	} else {
		w.current->cfstate = CFS_READY;
		ctx.pc() += wait_instruction::SIZE;
	}
}

void execute_goto(OpContext &ctx, const goto_instruction &i)
{
	ctx.pc() += i.jump();
}

void execute_brbool(OpContext &ctx, const brbool_instruction &i)
{
	const qbrt_value &op(ctx.srcvalue(i.op));
	if (i.opcode() == OP_BRT && op.data.b
			|| i.opcode() == OP_BRF && ! op.data.b) {
		ctx.pc() += i.jump();
	} else {
		ctx.pc() += brbool_instruction::SIZE;
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
		default:
			cerr << "unsupport type comparison: " << a.type->id
				<< endl;
			break;
	}
	return false;
}

void execute_brcmp(OpContext &ctx, const brcmp_instruction &i)
{
	const qbrt_value &a(ctx.srcvalue(i.ra));
	const qbrt_value &b(ctx.srcvalue(i.rb));
	bool follow_jump(false);
	switch (i.opcode()) {
		case OP_BREQ:
			follow_jump = equal_value(a,b);
			break;
		case OP_BRNE:
			follow_jump = ! equal_value(a,b);
			break;
		default:
			cerr << "Unsupported comparison: " << i.opcode()
				<< "\n";
			return;
	}
	if (follow_jump) {
		ctx.pc() += i.jump();
	} else {
		ctx.pc() += brcmp_instruction::SIZE;
	}
}

void execute_brfail(OpContext &ctx, const brfail_instruction &i)
{
	const qbrt_value &op(ctx.srcvalue(i.op));
	bool is_failure(op.type->id == TYPE_FAILURE.id);
	int valtype(op.type->id);
	if (i.opcode() == OP_BRFAIL && is_failure
			|| i.opcode() == OP_BRNFAIL && ! is_failure) {
		ctx.pc() += i.jump();
	} else {
		ctx.pc() += brfail_instruction::SIZE;
	}
}

void execute_cfailure(OpContext &ctx, const cfailure_instruction &i)
{
	const char *failtype = fetch_string(ctx.resource(), i.hashtag_id);
	qbrt_value &result(ctx.dstvalue(SPECIAL_REG_RESULT));
	qbrt_value::fail(result, new Failure(failtype));
	ctx.pc() += cfailure_instruction::SIZE;
}

void execute_consti(OpContext &ctx, const consti_instruction &i)
{
	qbrt_value::i(ctx.dstvalue(i.reg), i.value);
	ctx.pc() += consti_instruction::SIZE;
}

void execute_move(OpContext &ctx, const move_instruction &i)
{
	qbrt_value &dst(ctx.dstvalue(i.dst));
	const qbrt_value &src(ctx.srcvalue(i.src));
	dst = src;
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
	qbrt_value &dst(ctx.dstvalue(i.dst));
	const qbrt_value &src(ctx.srcvalue(i.src));
	dst = src;
	ctx.pc() += copy_instruction::SIZE;
}

void execute_consts(OpContext &ctx, const consts_instruction &i)
{
	const char *str = fetch_string(ctx.resource(), i.string_id);
	qbrt_value::str(ctx.dstvalue(i.reg), str);
	ctx.pc() += consts_instruction::SIZE;
}

void execute_consthash(OpContext &ctx, const consthash_instruction &i)
{
	const char *hash = fetch_string(ctx.resource(), i.hash_id);
	qbrt_value::hashtag(ctx.dstvalue(i.reg), hash);
	ctx.pc() += consthash_instruction::SIZE;
}

void execute_lcontext(OpContext &ctx, const lcontext_instruction &i)
{
	const char *name = fetch_string(ctx.resource(), i.hashtag);
	qbrt_value &dst(ctx.dstvalue(i.reg));
	qbrt_value *src = ctx.get_context(name);
	if (src) {
		qbrt_value::ref(dst, *src);
	} else {
		Failure *f = NEW_FAILURE("unknown_context"
				, ctx.function_name(), ctx.pc());
		f->debug << "cannot find context variable: " << name;
		qbrt_value::fail(dst, f);
		ctx.worker().current->cfstate = CFS_FAILED;
		cerr << f->debug_msg() << endl;
	}

	ctx.pc() += lcontext_instruction::SIZE;
}

void execute_loadfunc(OpContext &ctx, const lfunc_instruction &i)
{
	const ResourceTable &resource(ctx.resource());
	const ModSym &modsym(fetch_modsym(resource, i.modsym));
	const char *modname = fetch_string(resource, modsym.mod_name);
	const char *fname = fetch_string(resource, modsym.sym_name);
	const Module *mod(find_module(ctx.worker(), modname));
	if (!mod) {
		cerr << "cannot find module: " << modname << endl;
		exit(1);
	}
	Failure *fail;
	Function qbrt(mod->fetch_function(fname));
	qbrt_value &dst(ctx.dstvalue(i.reg));
	c_function cf = NULL;
	if (qbrt) {
		qbrt_value::f(dst, new function_value(qbrt));
	} else {
		c_function cf = fetch_c_function(*mod, fname);
		if (cf) {
			qbrt_value::cfunc(dst, new cfunction_value(cf));
		} else {
			cerr << "could not find function: " << modname
				<<"/"<< fname << endl;
			fail = FAIL_NOFUNCTION(ctx.function_name(), ctx.pc());
			fail->debug << "could not find function: " << modname
				<<'.'<< fname;
			qbrt_value::fail(dst, fail);
			ctx.worker().current->cfstate = CFS_FAILED;
		}
	}
	ctx.pc() += lfunc_instruction::SIZE;
}

void execute_lpfunc(OpContext &ctx, const lpfunc_instruction &i)
{
	const ModSym &modsym(fetch_modsym(ctx.resource(), i.modsym));
	const char *modname =
		fetch_string(ctx.resource(), modsym.mod_name);
	const char *protoname =
		fetch_string(ctx.resource(), modsym.sym_name);
	const char *fname = fetch_string(ctx.resource(), i.funcname);
	const Module *mod = NULL;
	if (modname && *modname) {
		mod = find_module(ctx.worker(), modname);
	} else {
		mod = ctx.worker().current->function_call().mod;
	}

	Function qbrt(mod->fetch_protocol_function(protoname, fname));
	qbrt_value &dst(ctx.dstvalue(i.reg));
	qbrt_value::f(dst, new function_value(qbrt));
	ctx.pc() += lpfunc_instruction::SIZE;
}

void execute_newproc(OpContext &ctx, const newproc_instruction &i)
{
	Failure *f;
	qbrt_value &pid(ctx.dstvalue(i.pid));
	qbrt_value &func(ctx.dstvalue(i.func));

	ctx.pc() += newproc_instruction::SIZE;

	function_value *fval = func.data.f;
	qbrt_value::set_void(func);
	Worker &w(ctx.worker());

	FunctionCall *call = new FunctionCall(*fval);
	ProcessRoot *proc = new_process(w.app, call);
	qbrt_value::i(pid, proc->pid);
}

void execute_recv(OpContext &ctx, const recv_instruction &i)
{
	Worker &w(ctx.worker());
	if (w.current->proc->recv.empty()) {
		w.current->cfstate = CFS_PEERWAIT;
		return;
	}

	qbrt_value &dst(ctx.dstvalue(i.dst));
	qbrt_value *msg(w.current->proc->recv.pop());
	dst = *msg;
	ctx.pc() += recv_instruction::SIZE;
}

void execute_stracc(OpContext &ctx, const stracc_instruction &i)
{
	Failure *f;
	qbrt_value &dst(ctx.dstvalue(i.dst));
	const qbrt_value &src(ctx.srcvalue(i.src));

	int op_pc(ctx.pc());
	ctx.pc() += stracc_instruction::SIZE;

	CHECK_FAILURE(dst, dst);
	CHECK_FAILURE(dst, src);

	if (dst.type->id != VT_BSTRING) {
		f = FAIL_TYPE(ctx.function_name(), op_pc);
		f->debug << "stracc destination is not a string";
		f->exit_code = 1;
		return;
	}

	ostringstream out;
	switch (src.type->id) {
		case VT_BSTRING:
			*dst.data.str += *src.data.str;
			break;
		case VT_INT:
			out << src.data.i;
			*dst.data.str += out.str();
			break;
		case VT_VOID:
			f = FAIL_TYPE(ctx.function_name(), op_pc);
			f->debug << "cannot append void to string";
			cerr << f->debug_msg() << endl;
			qbrt_value::fail(dst, f);
			ctx.worker().current->cfstate = CFS_FAILED;
			break;
		default:
			f = FAIL_TYPE(ctx.function_name(), op_pc);
			f->debug << "stracc source type is not supported: "
				<< (int) src.type->id;
			cerr << f->debug_msg() << endl;
			qbrt_value::fail(dst, f);
			ctx.worker().current->cfstate = CFS_FAILED;
			break;
	}
}

void execute_loadtype(OpContext &ctx, const loadtype_instruction &i)
{
	const char *modname = fetch_string(ctx.resource(), i.modname);
	const char *type_name = fetch_string(ctx.resource(), i.type);
	const Module &mod(*find_module(ctx.worker(), modname));
	const Type *typ = mod.fetch_struct(type_name);
	qbrt_value &dst(ctx.dstvalue(i.reg));
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
	Worker &w(ctx.worker());
	qbrt_value &func_reg(ctx.dstvalue(i.func_reg));
	qbrt_value &output(ctx.dstvalue(i.result_reg));
	// increment pc so it's in the right place when we get back
	ctx.pc() += call_instruction::SIZE;

	call(w, output, func_reg);
}

void execute_return(OpContext &ctx, const return_instruction &i)
{
	Worker &w(ctx.worker());
	w.current->cfstate = CFS_COMPLETE;
}

void fail(OpContext &ctx, Failure *f)
{
	qbrt_value &result(ctx.dstvalue(SPECIAL_REG_RESULT));
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
	x[OP_CONSTI] = (executioner) execute_consti;
	x[OP_CONSTS] = (executioner) execute_consts;
	x[OP_CONSTHASH] = (executioner) execute_consthash;
	x[OP_IADD] = (executioner) execute_binaryop;
	x[OP_IDIV] = (executioner) execute_divide;
	x[OP_IMULT] = (executioner) execute_binaryop;
	x[OP_ISUB] = (executioner) execute_binaryop;
	x[OP_LFUNC] = (executioner) execute_loadfunc;
	x[OP_LCONTEXT] = (executioner) execute_lcontext;
	x[OP_LPFUNC] = (executioner) execute_lpfunc;
	x[OP_NEWPROC] = (executioner) execute_newproc;
	x[OP_RECV] = (executioner) execute_recv;
	x[OP_STRACC] = (executioner) execute_stracc;
	x[OP_LOADOBJ] = (executioner) execute_loadobj;
	x[OP_MOVE] = (executioner) execute_move;
	x[OP_REF] = (executioner) execute_ref;
	x[OP_COPY] = (executioner) execute_copy;
	x[OP_FORK] = (executioner) execute_fork;
	x[OP_GOTO] = (executioner) execute_goto;
	x[OP_BRF] = (executioner) execute_brbool;
	x[OP_BRT] = (executioner) execute_brbool;
	x[OP_BREQ] = (executioner) execute_brcmp;
	x[OP_BRNE] = (executioner) execute_brcmp;
	x[OP_BRFAIL] = (executioner) execute_brfail;
	x[OP_BRNFAIL] = (executioner) execute_brfail;
	x[OP_WAIT] = (executioner) execute_wait;
}

void execute_instruction(Worker &w, const instruction &i)
{
	uint8_t opcode(i.opcode());
	executioner x = EXECUTIONER[opcode];
	WorkerOpContext ctx(w);
	if (!x) {
		Failure *f = new Failure("invalidopcode", ctx.function_name(), ctx.pc());
		f->exit_code = 1;
		f->debug << "Opcode not implemented: " << (int) opcode;
		f->usage << "Internal program error";
		cerr << f->debug_msg() << endl;
		fail(ctx, f);
		return;
	}
	//cerr << "---------------------------\n";
	//cerr << "execute opcode: " << (int) opcode << endl;
	x(ctx, i);
	//inspect_call_frame(cerr, *w.task->cframe);
}

void override_function(Worker &w, function_value &funcval)
{
	Function func(funcval.func);
	int pfc_type(PFC_TYPE(func.header->fcontext));

	if (pfc_type == FCT_TRADITIONAL) {
		return;
	}

	string param_types;
	load_function_param_types(param_types, funcval);

	const ResourceTable &resource(w.current->function_call().mod->resource);

	const ProtocolResource *proto;
	proto = find_function_protocol(w, func);

	const char *protosym = fetch_string(resource, proto->name_idx);
	const char *funcname = fetch_string(resource, func.header->name_idx);

	Function overridef(find_override(w, func.mod->name.c_str(), protosym
				, funcname, param_types));
	if (overridef) {
		funcval.func = overridef;
	} else if (pfc_type == FCT_POLYMORPH) {
		// no override. reset the func to the protocol function
		// if it was previously overridden
		funcval.func = find_default_function(w, funcval.func);
	}
}

void qbrtcall(Worker &w, qbrt_value &res, function_value *f)
{
	if (!f) {
		cerr << "function is null\n";
		w.current->cfstate = CFS_FAILED;
		return;
	}
	override_function(w, *f);

	FunctionCall *call = new FunctionCall(*w.current, res, *f);
	w.current = call;

	WorkerOpContext ctx(w);
	const ResourceTable &resource(w.current->function_call().mod->resource);
	// check arguments
	for (uint16_t i(0); i<f->func.header->argc; ++i) {
		const qbrt_value *val(&ctx.srcvalue(PRIMARY_REG(i)));
		if (!val) {
			cerr << "wtf null value?\n";
		}
		const Type *valtype = val->type;
		const ParamResource &param(f->func.header->params[i]);
		const char *name = fetch_string(resource, param.name_idx);
		const ModSym &type(fetch_modsym(resource, param.type_idx));
		const char *type_mod = fetch_string(resource, type.mod_name);
		const char *type_name = fetch_string(resource, type.sym_name);
		if (valtype->module != type_mod || valtype->name != type_name) {
			cerr << "Type Mismatch: parameter " << name << '/' << i
				<< " expected to be " << type_mod
				<< type_name << ", instead received "
				<< valtype->module << ':' << valtype->name
				<< endl;
			exit(1);
		}
	}
}

void ccall(Worker &w, qbrt_value &res, cfunction_value &f)
{
	c_function cf = f.func;
	WorkerCContext ctx(w, f);
	cf(ctx, res);
}

void call(Worker &w, qbrt_value &res, qbrt_value &f)
{
	Failure *fail;
	switch (f.type->id) {
		case VT_FUNCTION:
			qbrtcall(w, res, f.data.f);
			break;
		case VT_CFUNCTION:
			ccall(w, res, *f.data.cfunc);
			break;
		case VT_FAILURE:
			fail = DUPE_FAILURE(*f.data.failure
					, w.current->function_call().name()
					, w.current->pc);
			qbrt_value::fail(res, fail);
			break;
		default:
			fail = FAIL_TYPE(w.current->function_call().name()
					, w.current->pc);
			fail->debug << "Unknown function type: "
				<< (int)f.type->id;
			qbrt_value::fail(res, fail);
			return;
	}
}

void core_print(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = &ctx.srcvalue(PRIMARY_REG(0));
	if (! val) {
		cout << "no param for print\n";
		return;
	}
	switch (val->type->id) {
		case VT_INT:
			cout << val->data.i;
			break;
		case VT_BSTRING:
			cout << *(val->data.str);
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
	out << "name/context/fcontext: " << f.header->name_idx << '/'
		<< f.header->context_idx << '/' << (int) f.header->fcontext
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
	inspect_function(out, fv.func);
	inspect_value_index(out, fv);
	return out;
}

ostream & inspect_cfunction_value(ostream &out, const cfunction_value &fv)
{
	out << "cfunction ";
	inspect_value_index(out, fv);
	return out;
}

ostream & inspect_ref(ostream &out, qbrt_value &ref)
{
	out << "ref:";
	qbrt_value &val(follow_ref(ref));
	return inspect(out, val);
}

ostream & inspect_list(ostream &out, const List *l) {
	out << '[';
	while (l) {
		inspect(out, l->value);
		out << ',';
		l = l->next;
	}
	out << "]\n";
	return out;
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
		case VT_BSTRING:
			out << *v.data.str;
			break;
		case VT_FAILURE:
			inspect_failure(out, *v.data.failure);
			break;
		case VT_FUNCTION:
			inspect_function_value(out, *v.data.f);
			break;
		case VT_CFUNCTION:
			inspect_cfunction_value(out, *v.data.cfunc);
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
		case VT_LIST:
			inspect_list(out, v.data.list);
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
	const qbrt_value *val = &ctx.srcvalue(0);
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
		case VT_BSTRING:
			t = &TYPE_BSTRING;
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
	const qbrt_value &pid(ctx.srcvalue(PRIMARY_REG(0)));
	const qbrt_value &src(ctx.srcvalue(PRIMARY_REG(1)));

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

void list_empty(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value *val = &ctx.srcvalue(PRIMARY_REG(0));
	if (! val) {
		cerr << "no param for list empty\n";
		return;
	}
	switch (val->type->id) {
		case VT_LIST:
			qbrt_value::b(out, empty(val->data.list));
			break;
		default:
			cerr << "empty arg not a list: " << (int) val->type->id
				<< endl;
			break;
	}
}

void list_head(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value &val = ctx.srcvalue(PRIMARY_REG(0));
	switch (val.type->id) {
		case VT_LIST:
			out = head(val.data.list);
			break;
		default:
			cerr <<"head arg not a list: "<< (int)val.type->id
				<< endl;
			break;
	}
}

void list_pop(OpContext &ctx, qbrt_value &out)
{
	qbrt_value &val(ctx.dstvalue(PRIMARY_REG(0)));
	switch (val.type->id) {
		case VT_LIST:
			qbrt_value::list(out, pop(val.data.list));
			break;
		default:
			cerr <<"pop arg not a list: "<< (int)val.type->id
				<< endl;
			break;
	}
}

void core_open(OpContext &ctx, qbrt_value &out)
{
	const qbrt_value &filename(ctx.srcvalue(PRIMARY_REG(0)));
	const qbrt_value &mode(ctx.srcvalue(PRIMARY_REG(0)));
	// these type checks should be done automatically...
	// once types are working.
	if (filename.type->id != VT_BSTRING) {
		cerr << "first argument to open is not a string\n";
		cerr << "argument is type: " << (int)filename.type->id << endl;
		exit(2);
	}
	if (mode.type->id != VT_BSTRING) {
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
	qbrt_value &stream(ctx.dstvalue(PRIMARY_REG(0)));
	if (stream.type->id != VT_STREAM) {
		cerr << "first argument to getline is not a stream\n";
		exit(2);
	}
	ctx.io(stream.data.stream->getline(out));
}

void core_write(OpContext &ctx, qbrt_value &out)
{
	qbrt_value &stream(ctx.dstvalue(PRIMARY_REG(0)));
	const qbrt_value &text(ctx.srcvalue(PRIMARY_REG(1)));
	if (stream.type->id != VT_STREAM) {
		cerr << "first argument to write is not a stream\n";
		exit(2);
	}
	if (text.type->id != VT_BSTRING) {
		cerr << "second argument to write is not a string\n";
		cerr << "argument is type: " << (int) text.type->id << endl;
		exit(2);
	}
	ctx.io(stream.data.stream->write(*text.data.str));
}

int main(int argc, const char **argv)
{
	if (argc < 2) {
		cerr << "an object name is required\n";
		return 0;
	}
	const char *objname = argv[1];
	init_executioners();

	Module *mod_core = new Module("core");
	add_c_function(*mod_core, "pid", core_pid);
	add_c_function(*mod_core, "send", core_send);
	add_c_function(*mod_core, "wid", core_wid);
	add_type(*mod_core, "Int", TYPE_INT);
	add_type(*mod_core, "String", TYPE_BSTRING);
	add_type(*mod_core, "ByteString", TYPE_BSTRING);

	Module *mod_list = new Module("list");
	add_c_function(*mod_list, "empty", list_empty);
	add_c_function(*mod_list, "head", list_head);
	add_c_function(*mod_list, "pop", list_pop);

	Module *mod_io = new Module("io");
	add_c_function(*mod_io, "print", core_print);
	add_c_function(*mod_io, "open", core_open);
	add_c_function(*mod_io, "write", core_write);
	add_c_function(*mod_io, "getline", core_getline);

	Application app;
	load_module(app, mod_core);
	load_module(app, mod_list);
	load_module(app, mod_io);
	Worker &w0(new_worker(app));
	Worker &w1(new_worker(app));

	const Module *main_module = load_module(app, objname);
	if (!main_module) {
		return 1;
	}

	Function qbrt_main(main_module->fetch_function("__main"));
	if (!qbrt_main) {
		cerr << "no __main function defined\n";
		return 1;
	}
	function_value *main_func = new function_value(qbrt_main);
	int main_func_argc(main_func->func.argc());
	if (main_func_argc >= 1) {
		qbrt_value::i(main_func->reg[0], argc - 1);
	}
	if (main_func_argc >= 2) {
		qbrt_value::list(main_func->reg[1], NULL);
		for (int i(1); i<argc; ++i) {
			qbrt_value arg;
			string strarg(argv[i]);
			qbrt_value::str(arg, argv[i]);
			cons(arg, main_func->reg[1].data.list);
		}
		qbrt_value::list(main_func->reg[1]
				, reverse(main_func->reg[1].data.list));
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
	FunctionCall *main_call = new FunctionCall(result, *main_func);
	qbrt_value::stream(*add_context(main_call, "stdin"), stream_stdin);
	qbrt_value::stream(*add_context(main_call, "stdout"), stream_stdout);
	ProcessRoot *main_proc = new_process(app, main_call);

	int tid1 = pthread_create(&w0.thread, &w0.thread_attr
			, launch_worker, &w0);
	int tid2 = pthread_create(&w1.thread, &w1.thread_attr
			, launch_worker, &w1);

	application_loop(app);

	return result.data.i;
}
