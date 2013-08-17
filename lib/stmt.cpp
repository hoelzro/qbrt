#include "asm.h"
#include "qbrt/stmt.h"
#include "qbrt/logic.h"
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/string.h"
#include "instruction/schedule.h"
#include "instruction/type.h"
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

void bind_stmt::set_function_context(uint8_t, AsmResource *)
{
	this->polymorph = new AsmPolymorph(*protocol);
	list< bindtype_stmt * >::const_iterator it(params->begin());
	for (; it!=params->end(); ++it) {
		this->polymorph->type.push_back((*it)->bindtype);
	}

	if (functions) {
		::set_function_context(*functions, FCT_POLYMORPH, polymorph);
	}
}

void bind_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void bind_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *protocol);

	if (this->functions) {
		::collect_resources(rs, *functions);
	}
	AsmModSymList::iterator it(this->polymorph->type.begin());
	for (; it!=polymorph->type.end(); ++it) {
		collect_modsym(rs, **it);
	}
	collect_resource(rs, *this->polymorph);
}

void bind_stmt::pretty(std::ostream &out) const
{
	out << "bind " << *protocol;
}

void bindtype_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *bindtype);
}

void bindtype_stmt::pretty(std::ostream &out) const
{
	out << "bindtype " << *bindtype;
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

void construct_stmt::set_function_context(uint8_t, AsmResource *data)
{
	construct = new AsmConstruct(name, *data);
	string field_types;
	dparam_stmt::collect(construct->fields, field_types, this->fields);
}

void construct_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	collect_resource(rs, *construct);
	if (fields) {
		// this should be safe right? :-P
		::collect_resources(rs, *(Stmt::List *) fields);
	}
}

void construct_stmt::pretty(ostream &out) const
{
	out << "construct " << name.value;
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

void datatype_stmt::set_function_context(uint8_t, AsmResource *)
{
	datatype = new AsmDataType(name);
	::set_function_context(*constructs, 0, datatype);
}

void datatype_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	collect_resource(rs, *datatype);
	::collect_resources(rs, *constructs);
}

void datatype_stmt::pretty(ostream &out) const
{
	out << "datatype " << name.value;
}

void dparam_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	collect_typespec(rs, *type);
}

void dparam_stmt::pretty(std::ostream &out) const
{
	out << "dparam " << name.value << " " << *type;
}

void dparam_stmt::collect(AsmParamList &apl, string &param_types
		, dparam_stmt::List *stmts)
{
	apl.clear();
	param_types = "";

	if (!stmts || stmts->empty()) {
		return;
	}

	bool first(true);
	ostringstream out;
	dparam_stmt::List::const_iterator it(stmts->begin());
	for (; it!=stmts->end(); ++it) {
		if (first) {
			first = false;
		} else {
			out << " -> ";
		}
		apl.push_back(new AsmParam((*it)->name, *(*it)->type));
		(*it)->type->serialize(out);
	}
	param_types = out.str();
}

void dfunc_stmt::set_function_context(uint8_t afc, AsmResource *ctx)
{
	AsmFunc *f = new AsmFunc(this->name, *this->result);
	f->regc = 0;
	f->stmts = code;
	switch (afc) {
		case FCT_TRADITIONAL:
			f->fcontext = PFC_NONE;
			break;
		case FCT_PROTOCOL:
			f->fcontext = abstract ? PFC_ABSTRACT : PFC_DEFAULT;
			break;
		case FCT_POLYMORPH:
			f->fcontext = PFC_OVERRIDE;
			break;
	}
	f->ctx = ctx;
	dparam_stmt::collect(f->params, f->param_types.value, params);
	f->argc = params ? params->size() : 0;
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
	collect_typespec(rs, *this->result);
	collect_string(rs, this->func->param_types);
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
	out << "func " << name.value << "/" << argc();
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

void protocol_stmt::set_function_context(uint8_t, AsmResource *)
{
	this->protocol = new AsmProtocol(name, typevar);

	::set_function_context(*functions, FCT_PROTOCOL, this->protocol);
}

void protocol_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void protocol_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	list< AsmString * >::iterator it(typevar->begin());
	for (; it!=typevar->end(); ++it) {
		collect_string(rs, **it);
	}

	if (this->functions) {
		::collect_resources(rs, *functions);
	}
	collect_resource(rs, *this->protocol);
}

void protocol_stmt::pretty(std::ostream &out) const
{
	out << "protocol " << name.value;
	list< AsmString * >::iterator it(typevar->begin());
	for (; it!=typevar->end(); ++it) {
		out << " " << (*it)->value;
	}
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

void if_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void if_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new if_instruction(check, *reg));
}

void if_stmt::pretty(std::ostream &out) const
{
	out << (check ? "if " : "ifnot ")
		<< *reg << " @" << label.name;
}

ifcmp_stmt * ifcmp_stmt::eq(AsmReg *a, AsmReg *b, const std::string &lbl)
{
	return new ifcmp_stmt(OP_IFEQ, a, b, lbl);
}

ifcmp_stmt * ifcmp_stmt::ne(AsmReg *a, AsmReg *b, const std::string &lbl)
{
	return new ifcmp_stmt(OP_IFNOTEQ, a, b, lbl);
}

void ifcmp_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*a);
	r->alloc(*b);
}

void ifcmp_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new ifcmp_instruction(opcode, *a, *b));
}

void ifcmp_stmt::pretty(std::ostream &out) const
{
	out << "ifcmp " << *a << ' ' << *b << " @" << label.name;
}

void iffail_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void iffail_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new iffail_instruction(check, *reg));
}

void iffail_stmt::pretty(std::ostream &out) const
{
	out << (check ? "iffail " : "ifnotfail ")
		<< *reg << " @" << label.name;
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

void lconstruct_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *modsym);
}

void lconstruct_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lconstruct_stmt::generate_code(AsmFunc &f)
{
	if (modsym->module.value == "core") {
		if (modsym->symbol.value == "True") {
			asm_instruction(f, new copy_instruction(*dst
						, CONST_REG(REG_TRUE)));
			return;
		} else if (modsym->symbol.value == "False") {
			asm_instruction(f, new copy_instruction(*dst
						, CONST_REG(REG_FALSE)));
			return;
		}
	}
	asm_instruction(f, new lconstruct_instruction(*dst, *modsym->index));
}

void lconstruct_stmt::pretty(std::ostream &out) const
{
	out << "lconstruct " << *dst << " " << *modsym;
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

void match_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*result);
	r->alloc(*pattern);
	r->alloc(*input);
}

void match_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, nonmatch.name
			, new match_instruction(*result, *pattern, *input));
}

void match_stmt::pretty(std::ostream &out) const
{
	out << "match " << *result << ' ' << *pattern << ' ' << *input
		<< ' ' << nonmatch.name;
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

void patternvar_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void patternvar_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new patternvar_instruction(*dst));
}

void patternvar_stmt::pretty(ostream &out) const
{
	out << "patternvar " << *dst;
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
