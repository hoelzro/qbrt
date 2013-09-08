
#include "qbrt/core.h"
#include "qbrt/string.h"
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/module.h"
#include "qbrt/logic.h"
#include "instruction/schedule.h"
#include "instruction/type.h"
#include <vector>
#include <map>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <cstdlib>

using namespace std;


void print_register(reg_t reg)
{
	cout << ' ' << pretty_reg(reg);
}

void print_jump_delta(int16_t delta)
{
	if (delta > 0) {
		cout << "+";
	}
	cout << delta;
}

void print_binary_data(const uint8_t *data, uint16_t len)
{
	for (int x(0); x < len; ++x) {
		printf(" %02x", ((uint16_t) data[x]));
	}
	cout << endl;
}

typedef uint8_t (*instruction_printer)(const instruction &);

uint8_t print_binaryop_instruction(const binaryop_instruction &i)
{
	const char *opcode = "unknown";
	switch (i.opcode()) {
		case OP_IADD:
			opcode = "iadd";
			break;
		case OP_ISUB:
			opcode = "isub";
			break;
		case OP_IMULT:
			opcode = "imult";
			break;
		case OP_IDIV:
			opcode = "idiv";
			break;
	}
	cout << opcode;
	print_register(i.result);
	print_register(i.a);
	print_register(i.b);
	cout << endl;
	return binaryop_instruction::SIZE;
}

uint8_t print_call_instruction(const call_instruction &i)
{
	cout << "call";
	print_register(i.result_reg);
	print_register(i.func_reg);
	cout << endl;
	return call_instruction::SIZE;
}

uint8_t print_lcontext_instruction(const lcontext_instruction &i)
{
	cout << "lcontext " << pretty_reg(i.reg)
		<< " #" << i.hashtag << endl;
	return lcontext_instruction::SIZE;
}

uint8_t print_lconstruct_instruction(const lconstruct_instruction &i)
{
	cout << "lconstruct " << pretty_reg(i.reg)
		<< " modsym:" << i.modsym << endl;
	return lconstruct_instruction::SIZE;
}

uint8_t print_lfunc_instruction(const lfunc_instruction &i)
{
	cout << "lfunc " << pretty_reg(i.reg) << " modsym:" << i.modsym << endl;
	return lfunc_instruction::SIZE;
}

uint8_t print_match_instruction(const match_instruction &i)
{
	cout << "match";
	print_register(i.result);
	print_register(i.pattern);
	print_register(i.input);
	cout << ' ';
	print_jump_delta(i.jump_data);
	cout << endl;
	return match_instruction::SIZE;
}

uint8_t print_matchargs_instruction(const matchargs_instruction &i)
{
	cout << "matchargs";
	print_register(i.result);
	print_register(i.pattern);
	cout << ' ';
	print_jump_delta(i.jump_data);
	cout << endl;
	return matchargs_instruction::SIZE;
}

uint8_t print_newproc_instruction(const newproc_instruction &i)
{
	cout << "newproc " << pretty_reg(i.pid)
		<<' '<< pretty_reg(i.func) << endl;
	return newproc_instruction::SIZE;
}

uint8_t print_patternvar_instruction(const patternvar_instruction &i)
{
	cout << "patternvar " << pretty_reg(i.dst) << endl;
	return patternvar_instruction::SIZE;
}

uint8_t print_recv_instruction(const recv_instruction &i)
{
	cout << "recv " << pretty_reg(i.dst)
		<<' '<< pretty_reg(i.tube) << endl;
	return recv_instruction::SIZE;
}

uint8_t print_stracc_instruction(const stracc_instruction &i)
{
	cout << "stracc " << pretty_reg(i.dst)
		<<' '<< pretty_reg(i.src) << endl;
	return stracc_instruction::SIZE;
}

uint8_t print_loadobj_instruction(const loadobj_instruction &i)
{
	cout << "loadobj";
	cout << " s" << i.modname << endl;
	return loadobj_instruction::SIZE;
}

uint8_t print_move_instruction(const move_instruction &i)
{
	cout << "move";
	print_register(i.dst);
	print_register(i.src);
	cout << endl;
	return move_instruction::SIZE;
}

uint8_t print_ref_instruction(const ref_instruction &i)
{
	cout << "ref";
	print_register(i.dst);
	print_register(i.src);
	cout << endl;
	return ref_instruction::SIZE;
}

uint8_t print_copy_instruction(const copy_instruction &i)
{
	cout << "copy";
	print_register(i.dst);
	print_register(i.src);
	cout << endl;
	return copy_instruction::SIZE;
}

uint8_t print_consts_instruction(const consts_instruction &i)
{
	cout << "consts";
	print_register(i.reg);
	cout << " s" << i.string_id << endl;
	return consts_instruction::SIZE;
}

uint8_t print_consthash_instruction(const consthash_instruction &i)
{
	cout << "consthash";
	print_register(i.reg);
	cout << " #" << i.hash_id << endl;
	return consthash_instruction::SIZE;
}

uint8_t print_return_instruction(const return_instruction &i)
{
	cout << "ret\n";
	return return_instruction::SIZE;
}

uint8_t print_cfailure_instruction(const cfailure_instruction &i)
{
	cout << "cfailure";
	print_register(i.dst);
	cout << " #" << i.hashtag_id << endl;
	return cfailure_instruction::SIZE;
}

uint8_t print_consti_instruction(const consti_instruction &i)
{
	cout << "consti";
	print_register(i.reg);
	cout << " " << i.value << endl;
	return consti_instruction::SIZE;
}

uint8_t print_ctuple_instruction(const ctuple_instruction &i)
{
	cout << "ctuple";
	print_register(i.dst);
	cout << (int) i.size << endl;
	return ctuple_instruction::SIZE;
}

uint8_t print_fork_instruction(const fork_instruction &i)
{
	cout << "fork ";
	print_jump_delta(i.jump_data);
	print_register(i.result);
	cout << endl;
	return fork_instruction::SIZE;
}

uint8_t print_goto_instruction(const goto_instruction &i)
{
	cout << "goto ";
	print_jump_delta(i.jump_data);
	cout << endl;
	return goto_instruction::SIZE;
}

uint8_t print_if_instruction(const if_instruction &i)
{
	cout << (i.ifnot() ? "ifnot " : "if ");
	print_jump_delta(i.jump_data);
	print_register(i.op);
	cout << endl;
	return if_instruction::SIZE;
}

uint8_t print_ifcmp_instruction(const ifcmp_instruction &i)
{
	const char *bcname;
	switch (i.opcode()) {
		case OP_IFEQ:
			bcname = "ifeq ";
			break;
		case OP_IFNOTEQ:
			bcname = "ifnoteq ";
			break;
		default:
			bcname = "unk ";
			break;
	}
	print_jump_delta(i.jump_data);
	print_register(i.ra);
	print_register(i.rb);
	cout << endl;
	return ifcmp_instruction::SIZE;
}

uint8_t print_iffail_instruction(const iffail_instruction &i)
{
	cout << (i.iffail() ? "iffail " : "ifnotfail ");
	print_jump_delta(i.jump_data);
	print_register(i.op);
	cout << endl;
	return iffail_instruction::SIZE;
}

uint8_t print_wait_instruction(const wait_instruction &i)
{
	cout << "wait";
	print_register(i.reg);
	cout << endl;
	return wait_instruction::SIZE;
}

instruction_printer PRINTER[NUM_OP_CODES] = {0};

void set_printers()
{
	PRINTER[OP_CALL] = (instruction_printer) print_call_instruction;
	PRINTER[OP_CFAILURE] = (instruction_printer) print_cfailure_instruction;
	PRINTER[OP_IADD] = (instruction_printer) print_binaryop_instruction;
	PRINTER[OP_IMULT] = (instruction_printer) print_binaryop_instruction;
	PRINTER[OP_IDIV] = (instruction_printer) print_binaryop_instruction;
	PRINTER[OP_ISUB] = (instruction_printer) print_binaryop_instruction;
	PRINTER[OP_LCONTEXT] = (instruction_printer) print_lcontext_instruction;
	PRINTER[OP_LCONSTRUCT] =
		(instruction_printer) print_lconstruct_instruction;
	PRINTER[OP_LFUNC] = (instruction_printer) print_lfunc_instruction;
	PRINTER[OP_LOADOBJ] = (instruction_printer) print_loadobj_instruction;
	PRINTER[OP_MATCH] = (instruction_printer) print_match_instruction;
	PRINTER[OP_MATCHARGS] =
		(instruction_printer) print_matchargs_instruction;
	PRINTER[OP_CONSTS] = (instruction_printer) print_consts_instruction;
	PRINTER[OP_CONSTHASH] =
		(instruction_printer) print_consthash_instruction;
	PRINTER[OP_CTUPLE] = (instruction_printer) print_ctuple_instruction;
	PRINTER[OP_RETURN] = (instruction_printer) print_return_instruction;
	PRINTER[OP_MOVE] = (instruction_printer) print_move_instruction;
	PRINTER[OP_REF] = (instruction_printer) print_ref_instruction;
	PRINTER[OP_COPY] = (instruction_printer) print_copy_instruction;
	PRINTER[OP_CONSTI] = (instruction_printer) print_consti_instruction;
	PRINTER[OP_FORK] = (instruction_printer) print_fork_instruction;
	PRINTER[OP_GOTO] = (instruction_printer) print_goto_instruction;
	PRINTER[OP_IF] = (instruction_printer) print_if_instruction;
	PRINTER[OP_IFNOT] = (instruction_printer) print_if_instruction;
	PRINTER[OP_IFEQ] = (instruction_printer) print_ifcmp_instruction;
	PRINTER[OP_IFNOTEQ] = (instruction_printer) print_ifcmp_instruction;
	PRINTER[OP_IFFAIL] = (instruction_printer) print_iffail_instruction;
	PRINTER[OP_IFNOTFAIL] = (instruction_printer) print_iffail_instruction;
	PRINTER[OP_NEWPROC] = (instruction_printer) print_newproc_instruction;
	PRINTER[OP_PATTERNVAR] =
		(instruction_printer) print_patternvar_instruction;
	PRINTER[OP_RECV] = (instruction_printer) print_recv_instruction;
	PRINTER[OP_STRACC] = (instruction_printer) print_stracc_instruction;
	PRINTER[OP_WAIT] = (instruction_printer) print_wait_instruction;
}

void print_function_header(const FunctionHeader &f, const ResourceTable &tbl)
{
	const char *fctx = fcontext_name(f.fcontext);
	cout << "\n" << fctx << " function: ";

	const char *module = NULL;
	const char *pname = NULL;
	const ModSym *ms = NULL;
	const ProtocolResource *protocol = NULL;
	const PolymorphResource *poly = NULL;
	switch (f.fcontext) {
		case PFC_ABSTRACT:
		case PFC_DEFAULT:
			protocol = tbl.ptr< ProtocolResource >(f.context_idx);
			pname = fetch_string(tbl, protocol->name_idx);
			break;
		case PFC_OVERRIDE:
			poly = tbl.ptr< PolymorphResource >(f.context_idx);
			ms = &fetch_modsym(tbl, poly->protocol_idx);
			module = fetch_string(tbl, ms->mod_name);
			pname = fetch_string(tbl, ms->sym_name);
			break;
	}

	const char *fname = fetch_string(tbl, f.name_idx);
	if (module && *module) {
		cout << module << '/';
	}
	if (pname && *pname) {
		cout << pname << " ";
	}
	if (!(fname && *fname)) {
		cout << "fname is null?\n";
		return;
	}
	cout << fname << "/" << (int) f.argc << ',' << (int) f.regc << ":\n";

	const char *param_types = fetch_string(tbl, f.param_types_idx);
	cout << "function type: ";
	if (param_types && *param_types) {
		cout << param_types << " -> ";
	}
	const TypeSpecResource &result_type(
			tbl.obj< TypeSpecResource >(f.result_type_idx));
	const char *fullresult = fetch_string(tbl, result_type.fullname_idx);
	cout << fullresult << endl;

	if (poly) {
		cout << "types:";
		const char *typemod;
		const char *typesym;
		const char *fullname;
		const TypeSpecResource *tspec;
		for (int i(0); i<poly->type_count; ++i) {
			tspec = tbl.ptr< TypeSpecResource >(poly->type[i]);
			fullname = fetch_string(tbl, tspec->fullname_idx);
			cout << ' ' << fullname;
		}
		cout << endl;
	}
}

uint8_t print_instruction(const uint8_t *funcdata)
{
	const instruction &i(*(const instruction *) funcdata);
	instruction_printer p = PRINTER[i.opcode()];
	if (!p) {
		cerr << "\nnull printer for: " << (int) i.opcode() << endl;
		exit(1);
	}
	return p(i);
}

void print_function_code(const FunctionHeader &f, uint32_t size
		, const ResourceTable &tbl)
{
	if (f.fcontext == PFC_ABSTRACT) {
		// abstract, nothing to print
		return;
	}
	uint32_t pc(0);
	size -= FunctionHeader::SIZE;
	size -= f.argc * sizeof(ParamResource);
		// ^ spread b/n function header and code
	if (f.argc > 0) {
		cout << "args:\n";
		for (int i(0); i<f.argc; ++i) {
			const ParamResource &p(f.params[i]);
			const char *name = fetch_string(tbl, p.name_idx);
			const TypeSpecResource &type(
				tbl.obj< TypeSpecResource >(p.type_idx));
			const char *fullname =
				fetch_string(tbl, type.fullname_idx);
			cout << '\t' << name << ' ' << fullname << endl;
		}
	}

	cout << "code size:" << size << endl;
	const uint8_t *code(f.code());
	while (pc < size) {
		cout << pc << ":\t";
		cout.flush();
		print_instruction(code + pc);
		pc += isize(*(code + pc));
	}
}

void print_code(const ResourceTable &tbl)
{
	uint16_t i(0);
	for (; i<tbl.resource_count; ++i) {
		if (tbl.type(i) != RESOURCE_FUNCTION) {
			continue;
		}

		const FunctionHeader &f(
				tbl.obj< FunctionHeader >(i));
		print_function_header(f, tbl);
		cout << "offset:" << tbl.offset(i) << "\n";
		print_function_code(f, tbl.size(i), tbl);
	}
}

void print_constructure(const ConstructResource &conres
		, const ResourceTable &tbl)
{
	const char *name = fetch_string(tbl, conres.name_idx);
	cout << "\nconstruct: " << name << '/' << (int)conres.fld_count << endl;
	for (int i(0); i<conres.fld_count; ++i) {
		const ParamResource &p(conres.fields[i]);
		const char *name = fetch_string(tbl, p.name_idx);
		const TypeSpecResource &tsr(
				tbl.obj< TypeSpecResource >(p.type_idx));
		const char *fullname = fetch_string(tbl, tsr.fullname_idx);
		cout << '\t' << name <<' '<< fullname << endl;
	}
}

void print_constructs(const ResourceTable &tbl)
{
	uint16_t i(0);
	for (; i<tbl.resource_count; ++i) {
		if (tbl.type(i) != RESOURCE_CONSTRUCT) {
			continue;
		}

		const ConstructResource &conres(
				tbl.obj< ConstructResource >(i));
		print_constructure(conres, tbl);
	}
}


void print_hashtag(const ResourceTable &tbl, uint16_t index)
{
	const HashTagResource &hash(tbl.obj< HashTagResource >(index));
	printf("#%s\n", hash.value);
}

void print_import_line(const ResourceTable &tbl, uint16_t index)
{
	const ImportResource &import(tbl.obj< ImportResource >(index));
	printf("import %d\n", import.count);
}

void print_unsupported_resource(const ResourceTable &tbl, uint16_t i)
{
	uint16_t type(tbl.type(i));
	printf("unsupported:0x%x\n", type);
}

void print_escaped_string(const ResourceTable &tbl, uint16_t index)
{
	const StringResource &str(
			tbl.obj< StringResource >(index));
	ostringstream o;
	for (int i(0); i<str.length; ++i) {
		switch (str.value[i]) {
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
				o << str.value[i];
				break;
		}
	}
	printf("str(%u) \"%s\"\n", str.length, o.str().c_str());
}

void print_modsym(const ResourceTable &tbl, uint16_t index)
{
	const ModSym &modsym(tbl.obj< ModSym >(index));
	const StringResource &mod(tbl.obj< StringResource >(modsym.mod_name));
	const StringResource &sym(tbl.obj< StringResource >(modsym.sym_name));
	printf("modsym(%s/%s)\n", mod.value, sym.value);
}

void print_typespec(const ResourceTable &tbl, uint16_t index)
{
	const TypeSpecResource &typespec(tbl.obj< TypeSpecResource >(index));
	const StringResource &fullname(
			tbl.obj< StringResource >(typespec.fullname_idx));
	printf("typespec(%s)\n", fullname.value);
}

void print_construct(const ResourceTable &tbl, uint16_t index)
{
	const ConstructResource &cons(tbl.obj< ConstructResource >(index));
	const StringResource &name(tbl.obj< StringResource >(cons.name_idx));
	printf("construct %s/%d\n", name.value, cons.fld_count);
}

void print_datatype(const ResourceTable &tbl, uint16_t index)
{
	const DataTypeResource &dtr(tbl.obj< DataTypeResource >(index));
	const StringResource &name(tbl.obj< StringResource >(dtr.name_idx));
	printf("datatype %s/%d\n", name.value, dtr.argc);
}

void print_function_resource_line(const ResourceTable &tbl, uint16_t i)
{
	const FunctionHeader &f(tbl.obj< FunctionHeader >(i));
	const char *modname = NULL;
	const char *pname = NULL;
	const ProtocolResource *proto = NULL;
	const PolymorphResource *poly = NULL;
	const ModSym *proto_ms = NULL;
	const char *fname = fetch_string(tbl, f.name_idx);
	const char *fctx = fcontext_name(f.fcontext);
	printf("%s function ", fctx);

	switch (PFC_TYPE(f.fcontext)) {
		case FCT_PROTOCOL:
			proto = tbl.ptr< ProtocolResource >(f.context_idx);
			if (!proto) {
				cerr << "null protocol for function " << fname << endl;
				return;
			}
			pname = fetch_string(tbl, proto->name_idx);
			break;
		case FCT_POLYMORPH:
			poly = tbl.ptr< PolymorphResource >(f.context_idx);
			proto_ms = &fetch_modsym(tbl, poly->protocol_idx);
			modname = fetch_string(tbl, proto_ms->mod_name);
			pname = fetch_string(tbl, proto_ms->sym_name);
			break;
	}
	if (modname && *modname) {
		printf("%s/", modname);
	}
	if (pname && *pname) {
		printf("%s", pname);
	}
	printf(" %s/%d\n", fname, f.argc);
}

void print_protocol_resource_line(const ResourceTable &tbl, uint16_t i)
{
	const ProtocolResource &p(tbl.obj< ProtocolResource >(i));
	const StringResource &pname(tbl.obj< StringResource >(p.name_idx));
	printf("protocol %s/%u", pname.value, p.argc());

	const StringResource &prototype(
			tbl.obj< StringResource >(p.typevar_idx(0)));
	printf(" %s\n", prototype.value);
}

void print_polymorph_resource_line(const ResourceTable &tbl, uint16_t i)
{
	const PolymorphResource &poly(tbl.obj< PolymorphResource >(i));
	const ModSym &protoname(fetch_modsym(tbl, poly.protocol_idx));
	const char *modname(fetch_string(tbl, protoname.mod_name));
	const char *symname(fetch_string(tbl, protoname.sym_name));
	printf("polymorph ");
	if (modname && *modname) {
		printf("%s/", modname);
	}
	printf("%s", symname);

	for (uint16_t i(0); i<poly.type_count; ++i) {
		const TypeSpecResource &tspec(tbl.obj< TypeSpecResource >(
					poly.type[i]));
		const char *fullname(fetch_string(tbl, tspec.fullname_idx));
		printf(" %s", fullname);
	}
	printf("\n");
}

static inline void print_resource_line(const ResourceTable &tbl, uint16_t i)
{
	printf("\t% 2u ", i);
	switch (tbl.type(i)) {
		case RESOURCE_CONSTRUCT:
			print_construct(tbl, i);
			break;
		case RESOURCE_DATATYPE:
			print_datatype(tbl, i);
			break;
		case RESOURCE_FUNCTION:
			print_function_resource_line(tbl, i);
			break;
		case RESOURCE_HASHTAG:
			print_hashtag(tbl, i);
			break;
		case RESOURCE_IMPORT:
			print_import_line(tbl, i);
			break;
		case RESOURCE_MODSYM:
			print_modsym(tbl, i);
			break;
		case RESOURCE_POLYMORPH:
			print_polymorph_resource_line(tbl, i);
			break;
		case RESOURCE_PROTOCOL:
			print_protocol_resource_line(tbl, i);
			break;
		case RESOURCE_STRING:
			print_escaped_string(tbl, i);
			break;
		case RESOURCE_TYPESPEC:
			print_typespec(tbl, i);
			break;
		default:
			print_unsupported_resource(tbl, i);
			break;
	}
}

void print_resources(const ResourceTable &tbl)
{
	cout << "resources:\n";
	uint16_t index(0);
	for (uint16_t i(1); i<tbl.resource_count; ++i) {
		print_resource_line(tbl, i);
	}
}

void show_object_info(const char *objname)
{
	Module *mod = read_module(objname);
	if (!mod) {
		exit(1);
	}
	const ObjectHeader &header(mod->header);
	const ResourceTable &resource(mod->resource);

	string isapp(header.flags.f.application ? "yes" : "no");
	cout << "magic: " << header.magic << endl;
	cout << "application: " << isapp << endl;
	cout << "library: " << header.library_name << endl;
	cout << "version: " << header.library_version
		<< header.library_iteration << endl;
	cout << "bytes of code & data: " << resource.data_size << endl;
	cout << "# of resources: " << (resource.resource_count - 1) << endl;
	cout << hex;
	cout << "data offset: " << ResourceTable::DATA_OFFSET << endl;
	cout << "index offset: " << resource.index_offset() << endl;
	cout << dec;

	print_resources(resource);
	print_constructs(resource);
	print_code(resource);
}

int main(int argc, const char **argv)
{
	if (argc < 2) {
		cerr << "Not enough arguments\n";
		return 0;
	}

	init_instruction_sizes();
	set_printers();

	for (int i(1); i<argc; ++i) {
		show_object_info(argv[i]);
	}
	return 0;
}
