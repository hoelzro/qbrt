#include "asm.h"
#include "qbrt/stmt.h"
#include "qbrt/logic.h"
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/string.h"
#include "instruction/schedule.h"
#include <iostream>
#include <stdlib.h>

using namespace std;


uint32_t AsmString::write(std::ostream &o) const
{
	uint16_t strsize(value.size());
	o.write((const char *) &strsize, 2);
	o.write(value.c_str(), strsize);
	o.put('\0');
	return strsize + 3;
}

std::ostream & AsmString::pretty(std::ostream &o) const
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

uint32_t AsmHashTag::write(std::ostream &o) const
{
	uint16_t strsize(value.size());
	o.write((const char *) &strsize, 2);
	o.write(value.c_str(), strsize);
	o.put('\0');
	return strsize + 3;
}

std::ostream & AsmHashTag::pretty(std::ostream &o) const
{
	o << "#" << value;
	return o;
}


void Stmt::generate_code(AsmFunc &)
{
	cout << "no generate_code()";
}


void brbool_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void brbool_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brbool_instruction(check, 0, *reg));
}

void brbool_stmt::pretty(std::ostream &out) const
{
	out << (check ? "brt " : "brf ")
		<< *reg << " #" << label.name;
}

void brfail_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void brfail_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brfail_instruction(check, 0, *reg));
}

void brfail_stmt::pretty(std::ostream &out) const
{
	out << (check ? "brfail " : "brnfail ")
		<< *reg << " #" << label.name;
}

void binaryop_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*result);
	alloc->alloc(*a);
	alloc->alloc(*b);
}

void binaryop_stmt::generate_code(AsmFunc &f)
{
	uint16_t cmp;
	((char *) &cmp)[0] = type;
	((char *) &cmp)[1] = op;
	uint8_t opcode(0);
	switch (cmp) {
		case 0x2b69: // +i
			opcode = OP_IADD;
			break;
		case 0x2d69: // -i
			opcode = OP_ISUB;
			break;
		case 0x2a69: // *i
			opcode = OP_IMULT;
			break;
		case 0x2f69: // /i
			opcode = OP_IDIV;
			break;
		default:
			std::cerr << "unknown binary op: " << type << op
				<<' '<< cmp << endl;
			break;
	}
	asm_instruction(f, new binaryop_instruction(opcode, *result, *a, *b));
}

void binaryop_stmt::pretty(std::ostream &out) const
{
	out << "binaryop " << op << type <<' '<< *result
			<<' '<< *a <<' '<< *b;
}

brcmp_stmt * brcmp_stmt::ne(AsmReg *a, AsmReg *b, const std::string &lbl)
{
	return new brcmp_stmt(OP_BRNE, a, b, lbl);
}

void brcmp_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*a);
	r->alloc(*b);
}

void brcmp_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brcmp_instruction(opcode, *a, *b));
}

void brcmp_stmt::pretty(std::ostream &out) const
{
	out << "brcmp " << *a << ' ' << *b << label.name;
}

void call_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*result);
	r->alloc(*function);
}

void call_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new call_instruction(*result, *function));
}

void call_stmt::pretty(std::ostream &out) const
{
	out << "call " << *result
		<< " " << *function;
}

void cfailure_stmt::collect_resources(ResourceSet &rs)
{
	collect_resource(rs, type);
}

void cfailure_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new cfailure_instruction(*type.index));
}

void cfailure_stmt::pretty(std::ostream &out) const
{
	out << "cfailure #" << type.value;
}

void consti_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consti_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consti_instruction(*dst, value));
}

void consti_stmt::pretty(std::ostream &out) const
{
	out << "consti " << *dst << " " << value;
}

void consts_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consts_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, value);
}

void consts_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consts_instruction(*dst, *value.index));
}

void consts_stmt::pretty(std::ostream &out) const
{
	out << "consts " << *dst << " '" << value.value
		<< "'";
}

void consthash_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consthash_stmt::collect_resources(ResourceSet &rs)
{
	collect_resource(rs, value);
}

void consthash_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consthash_instruction(*dst, *value.index));
}

void consthash_stmt::pretty(std::ostream &out) const
{
	out << "consthash " << *dst << " #" << value.value;
}

void copy_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*src);
}

void copy_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new copy_instruction(*dst, *src));
}

void copy_stmt::pretty(std::ostream &out) const
{
	out << "copy " << *dst << " " << *src;
}

void dparam_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	collect_modsym(rs, *type);
}

void dparam_stmt::pretty(std::ostream &out) const
{
	out << "dparam " << name.value << " " << *type;
}

uint8_t dparam_stmt::collect(AsmParamList &apl, dparam_stmt::List *stmts)
{
	if (!stmts || stmts->empty()) {
		return 0;
	}
	dparam_stmt::List::const_iterator it(stmts->begin());
	for (; it!=stmts->end(); ++it) {
		apl.push_back(new AsmParam((*it)->name, *(*it)->type));
	}
	return stmts->size();
}

void dfunc_stmt::set_function_context(uint8_t afc, AsmResource *ctx)
{
	AsmFunc *f = new AsmFunc(this->name);
	f->regc = 0;
	f->stmts = code;
	f->ctx = ctx;
	f->ctx_type = afc;
	uint8_t collected_argc(dparam_stmt::collect(f->params, params));
	if (arity != collected_argc) {
		cerr << "Argument count mismatch\n";
		exit(1);
	}
	f->argc = collected_argc;
	this->func = f;
}

void dfunc_stmt::allocate_registers(RegAlloc *)
{
	if (!code) {
		return;
	}
	RegAlloc regs(func->argc);
	Stmt::List::iterator it(code->begin());
	for (; it!=code->end(); ++it) {
		(*it)->allocate_registers(&regs);
	}
	func->regc = regs.counter - func->argc;
}

void dfunc_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	if (params) {
		// this should be safe right? :-P
		::collect_resources(rs, *(Stmt::List *) params);
	}
	if (code) {
		::collect_resources(rs, *code);
	}
	collect_resource(rs, *this->func);
}

void dfunc_stmt::pretty(std::ostream &out) const
{
	out << "dfunc " << name.value << "/" << (uint16_t) arity;
}

void fork_stmt::allocate_registers(RegAlloc *ra)
{
	ra->alloc(*dst);
	if (code) {
		::allocate_registers(*code, ra);
	}
}

void fork_stmt::collect_resources(ResourceSet &rs)
{
	if (code) {
		::collect_resources(rs, *code);
	}
}

void fork_stmt::generate_code(AsmFunc &f)
{
	if (!code) {
		return;
	}
	asm_jump(f, "postfork", new fork_instruction(0, *dst));
	::generate_codeblock(f, *code);
	label_next(f, "postfork");
}

void dprotocol_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void dprotocol_stmt::set_function_context(uint8_t, AsmResource *)
{
	this->protocol = new AsmProtocol(name);
	this->protocol->argc = arity;

	::set_function_context(*functions, FCT_PROTOCOL, this->protocol);
}

void dprotocol_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	if (functions) {
		// :: prefix makes this call global func, not recurse
		::collect_resources(rs, *functions);
	}
	collect_resource(rs, *this->protocol);
}

void dprotocol_stmt::pretty(std::ostream &out) const
{
	out << "dprotocol " << name.value << "/" << (uint16_t) arity;
}

void dpolymorph_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void dpolymorph_stmt::set_function_context(uint8_t, AsmResource *)
{
	Stmt::List::iterator it(morph_stmts->begin());
	this->morph_types = new AsmModSymList();
	morphtype_stmt *mt;
	for (; it!=morph_stmts->end(); ++it) {
		mt = dynamic_cast< morphtype_stmt * >(*it);
		this->morph_types->push_back(mt->type);
	}

	this->polymorph = new AsmPolymorph(*protocol);
	this->polymorph->type = *this->morph_types;

	::set_function_context(*functions, FCT_POLYMORPH, this->polymorph);
}

void dpolymorph_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *protocol);
	if (this->morph_stmts) {
		::collect_resources(rs, *morph_stmts);
	}
	if (this->functions) {
		::collect_resources(rs, *functions);
	}

	collect_resource(rs, *this->polymorph);
}

void dpolymorph_stmt::pretty(std::ostream &out) const
{
	out << "dpolymorph " << *protocol;
}

void morphtype_stmt::pretty(std::ostream &out) const
{
	out << "morphtype " << *type;
}

void morphtype_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *type);
}

void fork_stmt::pretty(std::ostream &out) const
{
	out << "fork " << *dst;
}

void goto_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new goto_instruction(0));
}

void goto_stmt::pretty(std::ostream &out) const
{
	out << "goto #" << label.name;
}

void label_stmt::generate_code(AsmFunc &f)
{
	label_next(f, label.name);
}

void label_stmt::pretty(std::ostream &out) const
{
	out << "#" << label.name;
}

void lcontext_stmt::collect_resources(ResourceSet &rs)
{
	collect_resource(rs, name);
}

void lcontext_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lcontext_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new lcontext_instruction(*dst, *name.index));
}

void lcontext_stmt::pretty(std::ostream &out) const
{
	out << "lcontext " << *dst << " #" << name.value;
}

void lfunc_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *modsym);
}

void lfunc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lfunc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new lfunc_instruction(*dst, *modsym->index));
}

void lfunc_stmt::pretty(std::ostream &out) const
{
	out << "lfunc " << *dst << " " << *modsym;
}

void lpfunc_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *protocol);
	collect_string(rs, function);
}

void lpfunc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lpfunc_stmt::generate_code(AsmFunc &f)
{
	instruction *i;
	i = new lpfunc_instruction(*dst, *protocol->index, *function.index);
	asm_instruction(f, i);
}

void lpfunc_stmt::pretty(std::ostream &out) const
{
	out << "lpfunc " << dst <<' '<< *protocol
		<<' '<< function.value;
}

void newproc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*pid);
	r->alloc(*func);
}

void newproc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new newproc_instruction(*pid, *func));
}

void newproc_stmt::pretty(std::ostream &out) const
{
	out << "newproc " << *pid <<' '<< *func;
}

void recv_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*tube);
}

void recv_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new recv_instruction(*dst, *tube));
}

void recv_stmt::pretty(std::ostream &out) const
{
	out << "recv " << *dst <<' '<< *tube;
}

void ref_stmt::allocate_registers(RegAlloc *rc)
{
	rc->alloc(*dst);
	rc->alloc(*src);
}

void ref_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new ref_instruction(*dst, *src));
}

void ref_stmt::pretty(std::ostream &out) const
{
	out << "ref " << *dst << " " << *src;
}

void return_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new return_instruction);
}

void return_stmt::pretty(std::ostream &out) const
{
	out << "return";
}

void stracc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*src);
}

void stracc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new stracc_instruction(*dst, *src));
}

void stracc_stmt::pretty(std::ostream &out) const
{
	out << "stracc " << *dst <<' '<< *src;
}

void unimorph_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*pfreg);
	r->alloc(*valreg);
}

void unimorph_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new unimorph_instruction(*pfreg, *valreg));
}

void unimorph_stmt::pretty(std::ostream &out) const
{
	out << "morph " << *pfreg <<' '<< valreg;
}

void wait_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*reg);
}

void wait_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new wait_instruction(*reg));
}

void wait_stmt::pretty(std::ostream &out) const
{
	out << "wait " << *reg;
}
